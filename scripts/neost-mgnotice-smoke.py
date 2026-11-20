#!/usr/bin/env python3
"""Compare archived and rebuilt MGNOTICE note lifecycle under NeoST/MagiC.

Both binaries run in separate clones of the same MagiC image with the same
German RSC, EDITOBJC.SLB and delayed startup helper.  The scenario creates a
note with Ctrl-N, types "hello", leaves edit mode with Esc, quits with Ctrl-Q,
compares the saved note and every UI state, then reboots the same image to
verify that the note reopens.

The delayed startup helper is required because MagiC starts #_APP entries
before MAGXDESK owns the menu bar; MGNOTICE needs foreground windows for input.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import sys
import tempfile

from neost_common import acquire_headless, create_output, require, resolve_rom, run
from neost_image import MagiCImage
from neost_screen import diff_stats
from neost_server import NeoSTServer

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
ARCHIVE_APP = ROOT / "MagiC/APPS/MGNOTICE.2/BIN/MGNOTICE.APP"
ARCHIVE_RSC = ROOT / "MagiC/APPS/MGNOTICE.2/SRC/MGNOTICE.RSC"
ARCHIVE_SLB = ROOT / "MagiC/EXTENSIO/EDIT_SLB/BIN/EDITOBJC.SLB"
REBUILT_DIR = BUILD / "apps/mgnotice"
REBUILT_APP = REBUILT_DIR / "MGNOTICE.APP"
REBUILT_RSC = REBUILT_DIR / "mgnotice.rsc"
REBUILT_SLB = BUILD / "apps/edit-slb/EDITOBJC.SLB"
DRIVER = REBUILT_DIR / "NEOSTART.PRG"

BOOT_FRAMES = 15000
NOTE_DIR = r"::/GEMSYS/GEMDESK/NOTICE"
START_DIR = r"::/GEMSYS/MAGIC/START"
STATES = ("initial", "new", "typed", "escaped", "quit")


def start_server(headless: Path, rom: Path, image: Path, output: Path,
                 guest_root: Path) -> NeoSTServer:
    output.mkdir(parents=True)
    options = [
        "--machine", "megaste", "--mem", "4m", "--mono", "--no-disk",
        "--acsi", str(image), "--gemdos", str(guest_root),
        "--auto", r"C:\AUTO\MAGXBOOT.TOS",
    ]
    return NeoSTServer(headless, rom, output, options=options, timeout=180.0)


def run_lifecycle(headless: Path, rom: Path, image: Path,
                  output: Path) -> dict[str, Path]:
    guest_root = output / "GEMDRIVE"
    (guest_root / "E").mkdir(parents=True)
    shots = {name: output / f"{name}.ppm" for name in STATES}
    with start_server(headless, rom, image, output / "server", guest_root) as server:
        server.run_frames(BOOT_FRAMES)
        server.screenshot(shots["initial"])
        server.chord([0x1D, 0x31], frames=2)  # Ctrl-N
        server.run_frames(800)
        server.screenshot(shots["new"])
        for code in (0x23, 0x12, 0x26, 0x26, 0x18):  # HELLO
            server.press_key(code, frames=4)
        server.run_frames(400)
        server.screenshot(shots["typed"])
        server.press_key(0x01, frames=4)  # Esc leaves edit mode
        server.run_frames(600)
        server.screenshot(shots["escaped"])
        server.chord([0x1D, 0x10], frames=2)  # Ctrl-Q
        server.run_frames(1200)
        server.screenshot(shots["quit"])
    return shots


def run_reopen(headless: Path, rom: Path, image: Path,
               output: Path) -> Path:
    guest_root = output / "GEMDRIVE"
    (guest_root / "E").mkdir(parents=True)
    shot = output / "reopen.ppm"
    with start_server(headless, rom, image, output / "server", guest_root) as server:
        server.run_frames(BOOT_FRAMES)
        server.screenshot(shot)
    return shot


def extract_note(image: MagiCImage, destination: Path) -> None:
    image.copy_out(f"{NOTE_DIR}/0.NOT", destination)


def compare_states(archive: dict[str, Path], rebuilt: dict[str, Path]) -> None:
    for name in STATES:
        count, bbox = diff_stats(archive[name], rebuilt[name])
        require(count == 0, f"{name}: archived/rebuilt screens differ "
                            f"in {count} pixels, bbox {bbox}")
        print(f"{name}: screens identical")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--neost", type=Path,
                        default=Path.home() / ".local/bin/neost",
                        help="NeoST AppImage (default: ~/.local/bin/neost)")
    parser.add_argument("--neost-headless", type=Path,
                        help="direct path to the bundled neost-headless executable")
    environment_image = os.environ.get("MAGIC_IMAGE")
    environment_tos = os.environ.get("TOS206_ROM")
    parser.add_argument("--magic-image", type=Path,
                        default=Path(environment_image) if environment_image else None,
                        help="known-bootable MagiC image (or MAGIC_IMAGE)")
    parser.add_argument("--tos", type=Path,
                        default=Path(environment_tos) if environment_tos else None,
                        help="TOS 2.06 ROM (or TOS206_ROM)")
    parser.add_argument("--partition-offset", type=int, default=1024)
    parser.add_argument("--output", type=Path,
                        help="new output directory (default: timestamped build/atari path)")
    args = parser.parse_args()
    if args.magic_image is None or args.tos is None:
        parser.error("pass --magic-image and --tos (or set MAGIC_IMAGE and TOS206_ROM)")
    source = args.magic_image.expanduser().resolve()
    rom = args.tos.expanduser().resolve()
    for path in (source, rom):
        if not path.is_file():
            parser.error(f"input not found: {path}")

    build = run([str(ROOT / "scripts/atari-container"), "run", "make", "-f",
                 "Makefile.apps", "mgnotice", "mgnotice-test-driver",
                 "edit-slb"], cwd=ROOT, timeout=900)
    if build.returncode:
        print(build.stdout, file=sys.stderr)
        return build.returncode
    for path in (ARCHIVE_APP, ARCHIVE_RSC, ARCHIVE_SLB, REBUILT_APP,
                 REBUILT_RSC, REBUILT_SLB, DRIVER):
        if not path.is_file():
            print(f"build completed without producing {path}", file=sys.stderr)
            return 1

    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-mgnotice")
    except OSError as error:
        parser.error(str(error))

    shots: dict[str, dict[str, Path]] = {}
    notes: dict[str, Path] = {}
    try:
        with tempfile.TemporaryDirectory(prefix="neost-mgnotice-") as temporary:
            headless, _ = acquire_headless(args.neost_headless, args.neost,
                                           Path(temporary))
            for name, app, rsc, slb in (
                ("archive", ARCHIVE_APP, ARCHIVE_RSC, ARCHIVE_SLB),
                ("rebuilt", REBUILT_APP, REBUILT_RSC, REBUILT_SLB),
            ):
                directory = output / name
                directory.mkdir()
                image = MagiCImage.clone(source, directory / "magic.img",
                                         partition_offset=args.partition_offset)
                image.verify_magic()
                image.install(((app, f"{START_DIR}/MGNOTICE.APP"),
                               (rsc, f"{START_DIR}/mgnotice.rsc"),
                               (slb, f"{START_DIR}/EDITOBJC.SLB"),
                               (DRIVER, f"{START_DIR}/NEOSTART.PRG")))
                image.mkdir(NOTE_DIR)
                shots[name] = run_lifecycle(headless, rom, image.path,
                                            directory / "lifecycle")
                notes[name] = directory / "0.NOT"
                extract_note(image, notes[name])
                reopen = run_reopen(headless, rom, image.path, directory / "reopen")
                shots[name]["reopen"] = reopen
                print(f"{name}: lifecycle and reopen captured")

        compare_states(shots["archive"], shots["rebuilt"])
        # compare_states also checked reopen; compare note side effects separately.
        require(notes["archive"].read_bytes() == notes["rebuilt"].read_bytes(),
                "saved note files differ")
        text = notes["rebuilt"].read_bytes()
        require(b"BEGIN\r\nhello" in text, "rebuilt note does not contain hello")
        print(f"note: byte-identical ({len(text)} bytes) and contains hello")
        for name in ("archive", "rebuilt"):
            count, bbox = diff_stats(shots[name]["initial"], shots[name]["reopen"])
            require(count > 100, f"{name} reopen did not show a note window")
            require(bbox is not None and bbox[2] - bbox[0] < 100 and
                    bbox[3] - bbox[1] < 60,
                    f"{name} reopen changed more than the note window: {bbox}")
        count, bbox = diff_stats(shots["archive"]["reopen"],
                                 shots["rebuilt"]["reopen"])
        require(count == 0, f"reopen screens differ in {count} pixels, bbox {bbox}")
        print("reopen: note window visible and screens identical")

        print(f"MGNOTICE archived/rebuilt lifecycle passed; artifacts: {output}")
    except (OSError, RuntimeError) as error:
        print(f"NeoST MGNOTICE smoke failed: {error}\nArtifacts: {output}",
              file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
