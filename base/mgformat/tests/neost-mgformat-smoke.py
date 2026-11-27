#!/usr/bin/env python3
"""Compare archived and rebuilt MGFORMAT startup dialogs under MagiC.

Both binaries run from the pinned ``C:\\MGFORMAT.PRG`` guest path with the same
German resource. The delayed helper is installed in MagiC's START folder, and
the image's ``MAGX.INF`` is pinned to load that folder (any ``#_AUT`` is
removed), so the launcher cannot be skipped by a different source-image
startup path. The check proves the native MagiC wdlg binding and the format
dialog.

The compared regions are limited to the dialog title and controls; the
desktop's free-space status line and the editable name field's blinking caret
are excluded. A control boot with only ``C:\\MGFORMAT.PRG`` removed must still
boot but differ substantially in the dialog regions. NeoST's floppy formatter
reports a write fault for this disposable image, so physical formatting and
disk-copy effects remain a manual-hardware test.
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

from neost_common import acquire_headless, create_output, require, run
from neost_host import NeoSTHost
from neost_image import MagiCImage
from neost_screen import diff_stats

BUILD = ROOT / "build/atari"
ARCHIVE_APP = COMPONENT / "tests/MGFORMAT.PRG"
ARCHIVE_RSC = COMPONENT / "SRC/MGFORMAT.RSC"
REBUILT_APP = BUILD / "apps/mgformat/MGFORMAT.PRG"
REBUILT_RSC = BUILD / "apps/mgformat/MGFORMAT.RSC"
DRIVER = BUILD / "apps/mgformat/NEOSTART.PRG"
START_DIR = r"::/GEMSYS/MAGIC/START"
LAUNCHER_GUEST = f"{START_DIR}/NEOSTART.PRG"
# Keep in sync with base/mgformat/tests/NEOSTART.C: it Pexecs
# C:\MGFORMAT.PRG with the "-f A:" command tail.
APP_GUEST = r"::/MGFORMAT.PRG"
RSC_GUEST = r"::/MGFORMAT.RSC"
STARTUP_CONFIG_PATH = "C:\\GEMSYS\\MAGIC\\START\\"
AUTO_BOOT = r"C:\AUTO\MAGXBOOT.TOS"
BOOT_FRAMES = 15000
SETTLE_FRAMES = 600
SCREEN = (640, 400)

# The name field's caret at x=316 blinks, so its column (314..321) is split out.
REGIONS = {
    "dialog-title": (163, 54, 479, 106),
    "name-label": (163, 106, 314, 136),
    "name-field": (322, 106, 479, 136),
    "dialog-body": (163, 136, 479, 316),
}
PRIMARY_REGION = "dialog-body"
MIN_CONTROL_DIFF = 2000
MIN_PRIMARY_DIFF = 1000


def pin_launcher_config(image: MagiCImage, scratch: Path) -> None:
    """Pin MAGX.INF's startup folder and remove any autostart program."""
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
        if key.startswith("#_APP"):
            if not pinned:
                lines.append(f"#_APP {STARTUP_CONFIG_PATH}")
                pinned = True
            continue
        lines.append(line)
    if not pinned:
        # #_APP belongs to the [aes] section; insert after its header.
        pinned_lines: list[str] = []
        for line in lines:
            pinned_lines.append(line)
            if not pinned and line.strip().lower() == "#[aes]":
                pinned_lines.append(f"#_APP {STARTUP_CONFIG_PATH}")
                pinned = True
        require(pinned, "MAGX.INF has no [aes] section for the pinned app folder")
        lines = pinned_lines
    local.write_bytes((newline.join(lines) + newline).encode("latin-1"))
    image.copy_in(local, "::/MAGX.INF")


def remove_if_present(image: MagiCImage, guest: str) -> None:
    if image.exists(guest):
        image.remove(guest)


def capture(headless: Path, rom: Path, source: Path, directory: Path, *,
            app: Path | None, rsc: Path,
            partition_offset: int) -> tuple[Path, Path]:
    """Boot one case and return settled and final dialog screenshots."""
    directory.mkdir()
    host = NeoSTHost(
        headless, rom, directory,
        options=["--machine", "megaste", "--mem", "4m", "--mono", "--no-disk"],
        auto=AUTO_BOOT, timeout=240.0)
    image = host.prepare_image(source, partition_offset=partition_offset)
    pin_launcher_config(image, directory)
    (host.guest_root / "E").mkdir(parents=True, exist_ok=True)
    files = [(DRIVER, LAUNCHER_GUEST), (rsc, RSC_GUEST)]
    if app is not None:
        files.append((app, APP_GUEST))
    image.install(files)
    if app is None:
        remove_if_present(image, APP_GUEST)
    with host:
        host.boot(BOOT_FRAMES)
        host.server.move(-SCREEN[0], SCREEN[1], step=8, frames=1)
        host.run_frames(SETTLE_FRAMES)
        settled = host.screenshot("format-dialog.ppm")
        host.run_frames(SETTLE_FRAMES)
        final = host.screenshot("format-dialog-stable.ppm")
    return settled, final


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
            shots: dict[str, tuple[Path, Path]] = {}
            for name, app, rsc in (
                ("archive", ARCHIVE_APP, ARCHIVE_RSC),
                ("rebuilt", REBUILT_APP, REBUILT_RSC),
                ("missing", None, ARCHIVE_RSC),
            ):
                shots[name] = capture(
                    headless, rom, source, output / name, app=app, rsc=rsc,
                    partition_offset=args.partition_offset)

        for name in ("archive", "rebuilt"):
            settled, final = shots[name]
            for region_name, region in REGIONS.items():
                changed, bbox = diff_stats(settled, final, region=region)
                require(changed == 0,
                        f"{name} dialog changed between settled captures in "
                        f"{region_name}: {changed} pixels (bbox {bbox})")

        for region_name, region in REGIONS.items():
            changed, bbox = diff_stats(shots["archive"][1], shots["rebuilt"][1],
                                       region=region)
            require(changed == 0,
                    f"dialog {region_name} differs in {changed} pixels "
                    f"(bbox {bbox})")

        control = shots["missing"][1]
        for name in ("archive", "rebuilt"):
            counts = {
                region_name: diff_stats(shots[name][1], control,
                                        region=region)[0]
                for region_name, region in REGIONS.items()
            }
            total = sum(counts.values())
            require(counts[PRIMARY_REGION] >= MIN_PRIMARY_DIFF,
                    f"{name} dialog and missing-APP control differ by only "
                    f"{counts[PRIMARY_REGION]} pixels in {PRIMARY_REGION}")
            require(total >= MIN_CONTROL_DIFF,
                    f"{name} dialog and missing-APP control differ by only "
                    f"{total} pixels in the compared regions")
            print(f"{name}: missing-APP control differs in {total} pixels "
                  f"({counts[PRIMARY_REGION]} in {PRIMARY_REGION})")

        print("MGFORMAT startup dialog matches archive in the compared regions")
        (output / "comparison.txt").write_text(
            "PASS\n"
            f"archive={shots['archive'][1]}\n"
            f"rebuilt={shots['rebuilt'][1]}\n"
            f"missing-app-control={control}\n",
            encoding="utf-8")
        print(f"MGFORMAT dialog verified; artifacts: {output}")
    except (OSError, RuntimeError) as error:
        print(f"NeoST MGFORMAT smoke failed: {error}\nArtifacts: {output}",
              file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
