#!/usr/bin/env python3
"""Compare archived and rebuilt MAGXDESK v5 desktops under NeoST/MagiC.

Each executable is installed with the unchanged v5 German resources into a
separate clone of the same bootable image. The scenario boots MagiC and checks
the stable desktop content region, excluding the time-varying menu bar.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import sys
import tempfile

from neost_common import acquire_headless, create_output, require, run
from neost_image import MagiCImage
from neost_screen import diff_stats
from neost_server import NeoSTServer

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
SOURCE_DIR = ROOT / "MagiC/APPS/MAGICDESK/MAGXDESK.5"
ARCHIVE_APP = SOURCE_DIR / "MAGXDESK.APP"
RESOURCE = SOURCE_DIR / "MAGXDESK.RSC"
RESOURCE_LANGUAGE = SOURCE_DIR / "MAGXDESL.RSC"
REBUILT_APP = BUILD / "apps/magxdesk/MAGXDESK.APP"
GEMDESK_DIR = r"::/GEMSYS/GEMDESK"
BOOT_FRAMES = 15000


def run_desktop(headless: Path, rom: Path, image: Path,
                output: Path) -> Path:
    guest_root = output / "GEMDRIVE"
    guest_root.mkdir()
    server_dir = output / "server"
    server_dir.mkdir()
    screenshot = output / "desktop.ppm"
    options = [
        "--machine", "megaste", "--mem", "4m", "--mono", "--no-disk",
        "--acsi", str(image), "--gemdos", str(guest_root),
        "--auto", r"C:\AUTO\MAGXBOOT.TOS",
        "--console-log", str(output / "console.log"),
    ]
    with NeoSTServer(headless, rom, server_dir, options=options,
                     timeout=180.0) as server:
        server.run_frames(BOOT_FRAMES)
        server.screenshot(screenshot)
    return screenshot


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--neost", type=Path,
                        default=Path.home() / ".local/bin/neost",
                        help="NeoST AppImage (default: ~/.local/bin/neost)")
    parser.add_argument("--neost-headless", type=Path,
                        help="direct path to bundled neost-headless")
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
                        help="new evidence directory (default: build/atari path)")
    args = parser.parse_args()
    if args.magic_image is None or args.tos is None:
        parser.error("pass --magic-image and --tos (or set MAGIC_IMAGE and TOS206_ROM)")
    source = args.magic_image.expanduser().resolve()
    rom = args.tos.expanduser().resolve()
    for path in (source, rom):
        if not path.is_file():
            parser.error(f"input not found: {path}")

    build = run([str(ROOT / "scripts/atari-container"), "run", "make", "-f",
                 "Makefile.apps", "BUILD_DIR=build/atari/apps", "magxdesk"],
                cwd=ROOT, timeout=900)
    if build.returncode:
        print(build.stdout, file=sys.stderr)
        return build.returncode
    for path in (ARCHIVE_APP, RESOURCE, RESOURCE_LANGUAGE, REBUILT_APP):
        if not path.is_file():
            print(f"build completed without producing {path}", file=sys.stderr)
            return 1

    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-magxdesk")
    except OSError as error:
        parser.error(str(error))

    screenshots: dict[str, Path] = {}
    try:
        with tempfile.TemporaryDirectory(prefix="neost-magxdesk-") as temporary:
            headless, _ = acquire_headless(args.neost_headless, args.neost,
                                           Path(temporary))
            for name, app in (("archive", ARCHIVE_APP), ("rebuilt", REBUILT_APP)):
                directory = output / name
                directory.mkdir()
                image = MagiCImage.clone(
                    source, directory / "magic.img",
                    partition_offset=args.partition_offset)
                image.verify_magic()
                image.install(((app, f"{GEMDESK_DIR}/MAGXDESK.APP"),
                               (RESOURCE, f"{GEMDESK_DIR}/MAGXDESK.RSC"),
                               (RESOURCE_LANGUAGE, f"{GEMDESK_DIR}/MAGXDESL.RSC")))
                screenshots[name] = run_desktop(
                    headless, rom, image.path, directory)

        changed, bbox = diff_stats(screenshots["archive"], screenshots["rebuilt"],
                                   region=(0, 24, 640, 400))
        require(changed == 0,
                f"desktop content differs in {changed} pixels (bbox {bbox})")
        print("v5 desktop content: identical to archived build")
        (output / "comparison.txt").write_text(
            "PASS\n"
            f"archive={screenshots['archive']}\n"
            f"rebuilt={screenshots['rebuilt']}\n",
            encoding="utf-8")
        print(f"Evidence: {output}")
    except RuntimeError as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
