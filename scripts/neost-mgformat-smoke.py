#!/usr/bin/env python3
"""Compare archived and rebuilt MGFORMAT startup dialogs under MagiC.

Both binaries run from C:\\ with the same German resource.  The delayed helper
is installed in MagiC's START folder and launches MGFORMAT with ``-f A:``.
The check proves the native MagiC wdlg binding and the format dialog.  NeoST's
floppy formatter reports a write fault for this disposable image, so physical
formatting and disk-copy effects remain a manual-hardware test.
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
ARCHIVE_APP = ROOT / "MagiC/APPS/MGFORMAT/BIN/MGFORMAT.PRG"
ARCHIVE_RSC = ROOT / "MagiC/APPS/MGFORMAT/SRC/MGFORMAT.RSC"
REBUILT_APP = BUILD / "apps/mgformat/MGFORMAT.PRG"
REBUILT_RSC = BUILD / "apps/mgformat/MGFORMAT.RSC"
DRIVER = BUILD / "apps/mgformat/NEOSTART.PRG"
START_DIR = r"::/GEMSYS/MAGIC/START"
BOOT_FRAMES = 15000
STATUS_REGION = (70, 35, 250, 60)


def start_server(headless: Path, rom: Path, image: Path, output: Path,
                 guest_root: Path) -> NeoSTServer:
    output.mkdir(parents=True)
    options = [
        "--machine", "megaste", "--mem", "4m", "--mono", "--no-disk",
        "--acsi", str(image), "--gemdos", str(guest_root),
        "--auto", r"C:\AUTO\MAGXBOOT.TOS",
    ]
    return NeoSTServer(headless, rom, output, options=options, timeout=240.0)


def capture(headless: Path, rom: Path, image: Path, directory: Path) -> Path:
    directory.mkdir(exist_ok=True)
    guest_root = directory / "GEMDRIVE"
    (guest_root / "E").mkdir(parents=True)
    shot = directory / "format-dialog.ppm"
    with start_server(headless, rom, image, directory / "run", guest_root) as server:
        server.run_frames(BOOT_FRAMES)
        server.screenshot(shot)
    return shot


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
                 "Makefile.apps", "mgformat", "mgformat-test-driver"],
                cwd=ROOT, timeout=900)
    if build.returncode:
        print(build.stdout, file=sys.stderr)
        return build.returncode
    for path in (ARCHIVE_APP, ARCHIVE_RSC, REBUILT_APP, REBUILT_RSC, DRIVER):
        if not path.is_file():
            print(f"build completed without producing {path}", file=sys.stderr)
            return 1

    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-mgformat")
    except OSError as error:
        parser.error(str(error))

    try:
        with tempfile.TemporaryDirectory(prefix="neost-mgformat-") as temporary:
            headless, _ = acquire_headless(args.neost_headless, args.neost,
                                           Path(temporary))
            shots = {}
            for name, app, rsc in (
                ("archive", ARCHIVE_APP, ARCHIVE_RSC),
                ("rebuilt", REBUILT_APP, REBUILT_RSC),
            ):
                directory = output / name
                image = MagiCImage.clone(source, directory / "magic.img",
                                         partition_offset=args.partition_offset)
                image.verify_magic()
                image.install(((DRIVER, f"{START_DIR}/NEOSTART.PRG"),
                               (app, "::/MGFORMAT.PRG"),
                               (rsc, "::/MGFORMAT.RSC")))
                shots[name] = capture(headless, rom, image.path, directory)

        count, bbox = diff_stats(shots["archive"], shots["rebuilt"])
        require(count < 1000,
                f"startup dialogs differ in {count} pixels, bbox {bbox}")
        require(bbox is None or
                (STATUS_REGION[0] <= bbox[0] <= bbox[2] < STATUS_REGION[2] and
                 STATUS_REGION[1] <= bbox[1] <= bbox[3] < STATUS_REGION[3]),
                f"unexpected startup difference outside the status area: {bbox}")
        print(f"MGFORMAT startup dialog matches archive; differences: {count} pixels")
        print(f"MGFORMAT dialog verified; artifacts: {output}")
    except (OSError, RuntimeError) as error:
        print(f"NeoST MGFORMAT smoke failed: {error}\nArtifacts: {output}",
              file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
