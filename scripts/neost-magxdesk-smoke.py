#!/usr/bin/env python3
"""Compare archived and rebuilt MAGXDESK v5 desktops under NeoST/MagiC.

Each executable is installed with the unchanged v5 German resources into a
separate clone of the same bootable image. MAGX.INF is pinned to start the
installed MAGXDESK as the default shell (and any #_AUT login is removed), so
the desktop under test cannot come from the image's own shell configuration.

The comparison covers the desktop window chrome, the file area and the desktop
icons. It excludes the menu bar (clock) and the free-space status line, parks
the mouse outside the compared regions, and requires the settled desktop to be
stable before it compares anything. A third boot with only the APP removed is
the control: it must still boot, while its screen must differ substantially in
the compared regions.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import sys
import tempfile

from neost_common import acquire_headless, create_output, require, run
from neost_host import NeoSTHost
from neost_image import MagiCImage
from neost_screen import diff_stats

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
SOURCE_DIR = ROOT / "MagiC/APPS/MAGICDESK/MAGXDESK.5"
ARCHIVE_APP = SOURCE_DIR / "MAGXDESK.APP"
RESOURCE = SOURCE_DIR / "MAGXDESK.RSC"
RESOURCE_LANGUAGE = SOURCE_DIR / "MAGXDESL.RSC"
REBUILT_APP = BUILD / "apps/magxdesk/MAGXDESK.APP"
GEMDESK_DIR = r"::/GEMSYS/GEMDESK"
SHELL_APP_GUEST = f"{GEMDESK_DIR}/MAGXDESK.APP"
SHELL_CONFIG_PATH = "C:\\GEMSYS\\GEMDESK\\MAGXDESK.APP"
AUTO_BOOT = r"C:\AUTO\MAGXBOOT.TOS"
BOOT_FRAMES = 15000
SETTLE_FRAMES = 600
SCREEN = (640, 400)

# Menu bar (clock/keyboard indicator) and the "Bytes in N objects" line are not
# stable application output. The mouse is parked bottom-left before capture, so
# it cannot appear in these regions either.
REGIONS = {
    "window-title": (0, 18, 320, 39),
    "window-content": (0, 56, 320, 220),
    "desktop-icons": (500, 18, 640, 220),
}
PRIMARY_REGION = "window-content"
MIN_CONTROL_DIFF = 1000
MIN_PRIMARY_DIFF = 500


def pin_shell_config(image: MagiCImage, scratch: Path) -> None:
    """Make MAGX.INF start the installed APP as the shell, without #_AUT."""
    local = scratch / "MAGX.INF"
    image.copy_out("::/MAGX.INF", local)
    data = local.read_bytes().decode("latin-1")
    if "\r\n" in data:
        newline = "\r\n"
    elif "\r" in data:
        newline = "\r"
    else:
        newline = "\n"
    lines: list[str] = []
    pinned = False
    for line in data.splitlines():
        key = line.lstrip().upper()
        if key.startswith("#_AUT"):
            continue
        if key.startswith("#_SHL"):
            if not pinned:
                lines.append(f"#_SHL {SHELL_CONFIG_PATH}")
                pinned = True
            continue
        if key.startswith("#_CTR") and not pinned:
            lines.append(f"#_SHL {SHELL_CONFIG_PATH}")
            pinned = True
        lines.append(line)
    require(pinned, "MAGX.INF has no #_CTR marker for the pinned shell")
    local.write_bytes((newline.join(lines) + newline).encode("latin-1"))
    image.copy_in(local, "::/MAGX.INF")


def remove_if_present(image: MagiCImage, guest: str) -> None:
    if image.exists(guest):
        image.remove(guest)


def run_desktop(headless: Path, rom: Path, source: Path, directory: Path, *,
                app: Path | None, partition_offset: int) -> tuple[Path, Path]:
    """Boot one case and return settled and final desktop screenshots."""
    directory.mkdir()
    host = NeoSTHost(
        headless, rom, directory,
        options=["--machine", "megaste", "--mem", "4m", "--mono", "--no-disk",
                 "--console-log", str(directory / "console.log")],
        auto=AUTO_BOOT, timeout=180.0)
    image = host.prepare_image(source, partition_offset=partition_offset)
    pin_shell_config(image, directory)
    files = [(RESOURCE, f"{GEMDESK_DIR}/MAGXDESK.RSC"),
             (RESOURCE_LANGUAGE, f"{GEMDESK_DIR}/MAGXDESL.RSC")]
    if app is not None:
        files.append((app, SHELL_APP_GUEST))
    image.install(files)
    if app is None:
        remove_if_present(image, SHELL_APP_GUEST)
    with host:
        host.boot(BOOT_FRAMES)
        host.server.move(-SCREEN[0], SCREEN[1], step=8, frames=1)
        host.run_frames(SETTLE_FRAMES)
        settled = host.screenshot("desktop.ppm")
        host.run_frames(SETTLE_FRAMES)
        final = host.screenshot("desktop-stable.ppm")
    return settled, final


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

    screenshots: dict[str, tuple[Path, Path]] = {}
    try:
        with tempfile.TemporaryDirectory(prefix="neost-magxdesk-") as temporary:
            headless, _ = acquire_headless(args.neost_headless, args.neost,
                                           Path(temporary))
            for name, app in (("archive", ARCHIVE_APP), ("rebuilt", REBUILT_APP),
                              ("missing", None)):
                screenshots[name] = run_desktop(
                    headless, rom, source, output / name, app=app,
                    partition_offset=args.partition_offset)

        for name in ("archive", "rebuilt"):
            settled, final = screenshots[name]
            for region_name, region in REGIONS.items():
                changed, bbox = diff_stats(settled, final, region=region)
                require(changed == 0,
                        f"{name} desktop changed between settled captures in "
                        f"{region_name}: {changed} pixels (bbox {bbox})")

        for region_name, region in REGIONS.items():
            changed, bbox = diff_stats(screenshots["archive"][1],
                                       screenshots["rebuilt"][1],
                                       region=region)
            require(changed == 0,
                    f"desktop {region_name} differs in {changed} pixels "
                    f"(bbox {bbox})")

        control = screenshots["missing"][1]
        for name in ("archive", "rebuilt"):
            counts = {
                region_name: diff_stats(screenshots[name][1], control,
                                        region=region)[0]
                for region_name, region in REGIONS.items()
            }
            total = sum(counts.values())
            require(counts[PRIMARY_REGION] >= MIN_PRIMARY_DIFF,
                    f"{name} desktop and missing-APP control differ by only "
                    f"{counts[PRIMARY_REGION]} pixels in {PRIMARY_REGION}")
            require(total >= MIN_CONTROL_DIFF,
                    f"{name} desktop and missing-APP control differ by only "
                    f"{total} pixels in the compared regions")
            print(f"{name}: missing-APP control differs in {total} pixels "
                  f"({counts[PRIMARY_REGION]} in {PRIMARY_REGION})")

        print("v5 desktop content: identical to archived build")
        (output / "comparison.txt").write_text(
            "PASS\n"
            f"archive={screenshots['archive'][1]}\n"
            f"rebuilt={screenshots['rebuilt'][1]}\n"
            f"missing-app-control={control}\n",
            encoding="utf-8")
        print(f"Evidence: {output}")
    except RuntimeError as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
