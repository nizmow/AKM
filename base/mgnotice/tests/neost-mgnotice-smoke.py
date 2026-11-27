#!/usr/bin/env python3
"""Compare archived and rebuilt MGNOTICE note lifecycle under NeoST/MagiC.

Both binaries run in separate clones of the same MagiC image with the same
German RSC, EDITOBJC.SLB and delayed startup helper.  The scenario creates a
note with Ctrl-N, types "hello", leaves edit mode with Esc, quits with Ctrl-Q,
compares the saved note and every UI state, then reboots the same image to
verify that the note reopens.

MGNOTICE opens EDITOBJC.SLB with a NULL path, so the tested library is installed
in MagiC's default XTENSION search directory after removing SLBPATH overrides
and preload entries; inherited notes are cleared before each run.  A control
run with the library removed must produce neither a note nor the application
window.

The delayed startup helper is required because MagiC starts #_APP entries
before MAGXDESK owns the menu bar; MGNOTICE needs foreground windows for input.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import sys
import tempfile

COMPONENT = Path(__file__).resolve().parents[1]
ROOT = COMPONENT.parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from neost_common import acquire_headless, create_output, require, run
from neost_image import MagiCImage
from neost_screen import diff_stats
from neost_server import NeoSTServer

BUILD = ROOT / "build/atari"
ARCHIVE_APP = COMPONENT / "tests/MGNOTICE.APP"
ARCHIVE_RSC = COMPONENT / "SRC/MGNOTICE.RSC"
ARCHIVE_SLB = COMPONENT.parent / "edit-slb/tests/EDITOBJC.SLB"
REBUILT_DIR = BUILD / "apps/mgnotice"
REBUILT_APP = REBUILT_DIR / "MGNOTICE.APP"
REBUILT_RSC = REBUILT_DIR / "mgnotice.rsc"
REBUILT_SLB = BUILD / "apps/edit-slb/EDITOBJC.SLB"
DRIVER = REBUILT_DIR / "NEOSTART.PRG"

BOOT_FRAMES = 15000
NOTE_DIR = r"::/GEMSYS/GEMDESK/NOTICE"
START_DIR = r"::/GEMSYS/MAGIC/START"
STATES = ("initial", "new", "typed", "escaped", "quit")
EDITOBJC = "EDITOBJC.SLB"
# MGNOTICE calls Slbopen with a NULL path, so MagiC searches SLBPATH (or the
# default XTENSION directory). The tested library must live there, not in
# START, and no inherited copy may be preloaded or found on another path.
SLB_DIR = r"::/GEMSYS/MAGIC/XTENSION"
SLB_PATH = f"{SLB_DIR}/{EDITOBJC}"


def start_server(headless: Path, rom: Path, image: Path, output: Path,
                 guest_root: Path) -> NeoSTServer:
    output.mkdir(parents=True)
    options = [
        "--machine", "megaste", "--mem", "4m", "--mono", "--no-disk",
        "--acsi", str(image), "--gemdos", str(guest_root),
        "--auto", r"C:\AUTO\MAGXBOOT.TOS",
    ]
    return NeoSTServer(headless, rom, output, options=options, timeout=180.0)


def configure_inf(image: MagiCImage, output: Path) -> None:
    """Pin the library lookup to a cleaned default search directory."""
    inf = output / "MAGX.INF"
    image.copy_out("::/MAGX.INF", inf)
    data = inf.read_bytes()
    # A declared SLBPATH replaces the default XTENSION directory and could
    # supply an inherited same-name library, so remove it.
    data = re.sub(rb"(?im)^[ \t]*#_ENV[ \t]+SLBPATH=.*(?:\r?\n|$)", b"", data)
    require(not re.search(rb"(?im)^[ \t]*#_ENV[ \t]+SLBPATH=", data),
            "MAGX.INF still declares SLBPATH")
    # A preloaded library is returned by name before any search path is used.
    active = re.compile(rb"(?im)^[ \t]*#_SLB[ \t]+\d+[ \t]+" +
                        re.escape(EDITOBJC.encode()) + rb"[ \t]*\r?\n?")
    data = active.sub(b"", data)
    require(not active.search(data), f"MAGX.INF still preloads {EDITOBJC}")
    inf.write_bytes(data)
    image.copy_in(inf, "::/MAGX.INF")


def clear_notes(image: MagiCImage, directory: Path) -> None:
    """Start each case without note files inherited from the input image."""
    image.remove(NOTE_DIR, tree=True)
    image.mkdir(NOTE_DIR)
    require(not image.exists(f"{NOTE_DIR}/0.NOT"),
            f"{directory.name}: inherited note evidence survived removal")


def prepare_case(source: Path, directory: Path, app: Path, rsc: Path,
                 slb: Path | None, partition_offset: int) -> MagiCImage:
    directory.mkdir()
    image = MagiCImage.clone(source, directory / "magic.img",
                             partition_offset=partition_offset)
    image.verify_magic()
    configure_inf(image, directory)
    files = [(app, f"{START_DIR}/MGNOTICE.APP"),
             (rsc, f"{START_DIR}/mgnotice.rsc"),
             (DRIVER, f"{START_DIR}/NEOSTART.PRG")]
    if slb is None:
        image.remove(SLB_PATH)
        require(not image.exists(SLB_PATH),
                "inherited EDITOBJC.SLB survived removal")
    else:
        files.append((slb, SLB_PATH))
    image.install(files)
    if slb is not None:
        require(image.exists(SLB_PATH),
                "EDITOBJC.SLB was not installed on the lookup path")
    clear_notes(image, directory)
    return image


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


def run_missing_library(headless: Path, rom: Path, image: Path,
                        output: Path) -> dict[str, Path]:
    """Attempt note creation while EDITOBJC.SLB is absent from every path."""
    guest_root = output / "GEMDRIVE"
    (guest_root / "E").mkdir(parents=True)
    shots = {"initial": output / "initial.ppm",
             "attempt": output / "attempt.ppm"}
    with start_server(headless, rom, image, output / "server", guest_root) as server:
        server.run_frames(BOOT_FRAMES)
        server.screenshot(shots["initial"])
        server.chord([0x1D, 0x31], frames=2)  # Ctrl-N saves a new note at once
        server.run_frames(800)
        server.screenshot(shots["attempt"])
    return shots


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
                image = prepare_case(source, directory, app, rsc, slb,
                                     args.partition_offset)
                shots[name] = run_lifecycle(headless, rom, image.path,
                                            directory / "lifecycle")
                notes[name] = directory / "0.NOT"
                extract_note(image, notes[name])
                reopen = run_reopen(headless, rom, image.path, directory / "reopen")
                shots[name]["reopen"] = reopen
                print(f"{name}: lifecycle and reopen captured")

            # Control: the same app and input with the library removed from
            # every lookup path must not produce a note or its window.
            control_dir = output / "missing-library"
            control = prepare_case(source, control_dir, REBUILT_APP,
                                   REBUILT_RSC, None, args.partition_offset)
            shots["missing-library"] = run_missing_library(
                headless, rom, control.path, control_dir / "run")

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

        require(not control.exists(f"{NOTE_DIR}/0.NOT"),
                "missing EDITOBJC.SLB still produced a note")
        count, bbox = diff_stats(shots["archive"]["initial"],
                                 shots["missing-library"]["initial"])
        # With the library the initial screen carries MGNOTICE's menu; without
        # it the desktop menu remains.  The saved-note check above is the
        # primary control; this confirms the application UI is also absent.
        require(count > 100,
                f"missing EDITOBJC.SLB still showed MGNOTICE UI "
                f"({count} pixels differ, bbox {bbox})")
        print("missing library: no note and no application UI")

        print(f"MGNOTICE archived/rebuilt lifecycle passed; artifacts: {output}")
    except (OSError, RuntimeError) as error:
        print(f"NeoST MGNOTICE smoke failed: {error}\nArtifacts: {output}",
              file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
