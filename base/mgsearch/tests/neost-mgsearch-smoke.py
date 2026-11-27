#!/usr/bin/env python3
"""Compare archived and rebuilt MGSEARCH on identical MagiC fixtures.

Both binaries run in separate clones of the same image with the same German
resource, delayed launcher and C: fixture tree.  The scenario opens the search
dialog, types "*.ZZZ", submits it, and compares the dialog and result screens.
The fixture includes a nested directory so the result also covers recursive
search; only the defined screens are compared.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import sys
import tempfile

COMPONENT = Path(__file__).resolve().parents[1]
ROOT = COMPONENT.parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from neost_common import acquire_headless, create_output, require, resolve_rom, run
from neost_image import MagiCImage
from neost_screen import diff_stats
from neost_server import NeoSTServer

BUILD = ROOT / "build/atari"
ARCHIVE_APP = COMPONENT / "tests/MGSEARCH.APP"
RESOURCE = COMPONENT / "SRC/MGSEARCH.RSC"
REBUILT_DIR = BUILD / "apps/mgsearch"
REBUILT_APP = REBUILT_DIR / "MGSEARCH.APP"
DRIVER = REBUILT_DIR / "NEOSTART.PRG"
START_DIR = r"::/GEMSYS/MAGIC/START"
GEMDESK_DIR = r"::/GEMSYS/GEMDESK"
FIXTURE_DIR = r"::/SEARCHFIX"
BOOT_FRAMES = 15000
SEARCH_FRAMES = 3000


def start_server(headless: Path, rom: Path, image: Path, output: Path,
                 guest_root: Path) -> NeoSTServer:
    output.mkdir(parents=True)
    options = [
        "--machine", "megaste", "--mem", "4m", "--mono", "--no-disk",
        "--acsi", str(image), "--gemdos", str(guest_root),
        "--auto", r"C:\AUTO\MAGXBOOT.TOS",
    ]
    return NeoSTServer(headless, rom, output, options=options, timeout=180.0)


def run_search(headless: Path, rom: Path, image: Path, output: Path) -> dict[str, Path]:
    guest_root = output / "GEMDRIVE"
    guest_root.mkdir(parents=True)
    shots = {name: output / f"{name}.ppm"
             for name in ("initial", "typed", "result")}
    with start_server(headless, rom, image, output / "server", guest_root) as server:
        server.run_frames(BOOT_FRAMES)
        server.screenshot(shots["initial"])
        server.chord([0x2A, 0x09], frames=2)  # *
        server.press_key(0x34, frames=2)      # .
        for code in (0x2C, 0x2C, 0x2C):       # ZZZ
            server.press_key(code, frames=2)
        server.run_frames(100)
        server.screenshot(shots["typed"])
        server.press_key(0x1C, frames=4)      # Enter submits the dialog
        server.run_frames(SEARCH_FRAMES)
        server.screenshot(shots["result"])
    return shots


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
                 "Makefile.apps", "mgsearch", "mgsearch-test-driver"],
                cwd=ROOT, timeout=900)
    if build.returncode:
        print(build.stdout, file=sys.stderr)
        return build.returncode
    for path in (ARCHIVE_APP, RESOURCE, REBUILT_APP, DRIVER):
        if not path.is_file():
            print(f"build completed without producing {path}", file=sys.stderr)
            return 1

    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-mgsearch")
    except OSError as error:
        parser.error(str(error))

    screenshots: dict[str, dict[str, Path]] = {}
    try:
        with tempfile.TemporaryDirectory(prefix="neost-mgsearch-") as temporary:
            scratch = Path(temporary)
            headless, _ = acquire_headless(args.neost_headless, args.neost, scratch)
            for name, app in (("archive", ARCHIVE_APP), ("rebuilt", REBUILT_APP)):
                directory = output / name
                directory.mkdir()
                image = MagiCImage.clone(source, directory / "magic.img",
                                         partition_offset=args.partition_offset)
                image.verify_magic()
                image.install(((app, f"{GEMDESK_DIR}/MGSEARCH.APP"),
                               (RESOURCE, f"{GEMDESK_DIR}/MGSEARCH.RSC"),
                               (DRIVER, f"{START_DIR}/NEOSTART.PRG")))
                image.mkdir(FIXTURE_DIR)
                image.mkdir(f"{FIXTURE_DIR}/SUB")
                alpha = scratch / "ALPHA.ZZZ"
                beta = scratch / "BETA.ZZZ"
                alpha.write_text("alpha\n", encoding="ascii")
                beta.write_text("beta\n", encoding="ascii")
                image.copy_in(alpha, f"{FIXTURE_DIR}/ALPHA.ZZZ")
                image.copy_in(beta, f"{FIXTURE_DIR}/SUB/BETA.ZZZ")
                screenshots[name] = run_search(headless, rom, image.path,
                                               directory / "search")
        for state in ("initial", "typed", "result"):
            count = diff_stats(screenshots["archive"][state],
                               screenshots["rebuilt"][state])[0]
            require(count == 0, f"{state}: archived/rebuilt screens differ "
                                f"in {count} pixels")
            print(f"{state}: screens identical")
        dialog = diff_stats(screenshots["rebuilt"]["initial"],
                            screenshots["rebuilt"]["typed"])[0]
        result = diff_stats(screenshots["rebuilt"]["typed"],
                            screenshots["rebuilt"]["result"])[0]
        require(dialog > 0, "typing did not change the search dialog")
        require(result > 0, "submitting did not open the result window")
        print(f"state changes: typed={dialog} pixels, result={result} pixels")
        (output / "comparison.txt").write_text(
            "PASS\n"
            f"initial={screenshots['rebuilt']['initial']}\n"
            f"typed={screenshots['rebuilt']['typed']}\n"
            f"result={screenshots['rebuilt']['result']}\n",
            encoding="utf-8")
        print(f"Evidence: {output}")
    except RuntimeError as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
