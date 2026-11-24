#!/usr/bin/env python3
"""Build the German core ZIP twice, install it cleanly and boot it on MagiC."""
from __future__ import annotations

import argparse
import hashlib
import os
import shutil
import sys
import tempfile
import zipfile
from pathlib import Path

from neost_common import acquire_headless, create_output, require, run, sha256
from neost_host import NeoSTHost
from neost_image import MagiCImage
from neost_screen import diff_stats

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "build/atari"
RELEASE = BUILD / "release"
PACKAGE = ROOT / "scripts/make-release.py"
PAYLOAD = ("AUTO", "BIN", "EXTRAS", "GEMSYS", "LANG")
ROOT_FILES = ("MAGX.INF", "MAGIC.RAM", "AUTOEXEC.BAT", "INSTALL.TXT", "NEWDESK.INF")
BUILD_TARGETS = [
    "magxboot", "mfa2", "mcmd", "magxdesk", "applicat", "mgcopy", "mgsearch",
    "mgedit", "mgview", "mgnotice", "mgformat", "vt52", "vfatconf", "wbdaemon",
    "cops", "magxconf-cpx", "tslice-cpx", "winframe", "mod-app", "showfile",
    "load-img", "osd-batch",
]


def build_payload() -> None:
    steps = [
        [str(ROOT / "scripts/atari-container"), "run", "make", "-j4", "-f",
         "Makefile.atari", "magic"],
        [str(ROOT / "scripts/atari-container"), "run", "make", "-j4", "-f",
         "Makefile.apps", *BUILD_TARGETS],
    ]
    for command in steps:
        result = run(command, cwd=ROOT, timeout=1200)
        require(result.returncode == 0, f"build failed: {command[-3:]}\n{result.stdout[-2000:]}")


def install(source: Path, folder: Path, offset: int) -> tuple[MagiCImage, Path]:
    image = MagiCImage.clone(source, folder / "magic.img", partition_offset=offset)
    image.verify_magic()
    for name in PAYLOAD:
        run(["mdeltree", "-i", image.image_spec, f"::/{name}"])
    for name in ROOT_FILES:
        run(["mdel", "-i", image.image_spec, f"::/{name}"])
    extracted = folder / "extract"
    with zipfile.ZipFile(RELEASE / "core-de-1.zip") as archive:
        archive.extractall(extracted)
    result = run(["mcopy", "-s", "-o", "-i", image.image_spec,
                  *map(str, sorted(extracted.iterdir())), "::/"], timeout=300)
    require(result.returncode == 0, f"clean extraction failed: {result.stdout}")
    return image, extracted


def verify_members(image: MagiCImage, folder: Path) -> None:
    restored = folder / "verify"
    with zipfile.ZipFile(RELEASE / "core-de-1.zip") as archive:
        members = sorted(archive.namelist())
        for name in members:
            target = restored / name
            target.parent.mkdir(parents=True, exist_ok=True)
            result = run(["mcopy", "-o", "-i", image.image_spec, f"::/{name}", str(target)])
            require(result.returncode == 0, f"installed member missing: {name}")
            expected = hashlib.sha256(archive.read(name)).hexdigest()
            require(sha256(target) == expected, f"installed member differs from ZIP: {name}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--magic-image", type=Path, default=os.environ.get("MAGIC_IMAGE"))
    parser.add_argument("--tos", type=Path, default=os.environ.get("TOS206_ROM"))
    parser.add_argument("--neost", type=Path, default=Path.home() / ".local/bin/neost")
    parser.add_argument("--neost-headless", type=Path)
    parser.add_argument("--partition-offset", type=int, default=1024)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--skip-build", action="store_true")
    args = parser.parse_args()
    if args.magic_image is None or args.tos is None:
        parser.error("supply --magic-image/--tos or MAGIC_IMAGE/TOS206_ROM")
    source, rom = (path.expanduser().resolve() for path in (args.magic_image, args.tos))
    try:
        if not args.skip_build:
            build_payload()
        output = create_output(args.output, parent=BUILD, prefix="neost-release").resolve()
        print(f"Evidence: {output}", flush=True)
        (RELEASE).mkdir(exist_ok=True)
        manifest = output / "core-de.manifest"
        for name in ("core-de-1.zip", "core-de-2.zip"):
            result = run([sys.executable, str(PACKAGE), "--output", str(RELEASE / name)] +
                         (["--manifest", str(manifest)] if name.endswith("1.zip") else []),
                         cwd=ROOT, timeout=300)
            require(result.returncode == 0, result.stdout)
        require(sha256(RELEASE / "core-de-1.zip") == sha256(RELEASE / "core-de-2.zip"),
                "two package runs produced different ZIPs")
        image, extracted = install(source, output, args.partition_offset)
        verify_members(image, output)
        with tempfile.TemporaryDirectory(prefix="neost-release-") as scratch:
            headless, _ = acquire_headless(args.neost_headless, args.neost, Path(scratch))
            host = NeoSTHost(headless, rom, output / "run", image=image,
                             options=["--machine", "megaste", "--mem", "4m", "--mono",
                                      "--no-disk"],
                             auto=r"C:\AUTO\MAGXBOOT.TOS", timeout=240)
            (host.guest_root / "E").mkdir()
            with host:
                host.boot(15000)
                host.calibrate_mouse()
                host.move_to(408, 384)
                host.run_frames(10)
                desktop = host.screenshot(output / "desktop.ppm")
                host.move_to(40, 8)
                host.run_frames(30)
                host.click()
                host.run_frames(60)
                menu = host.screenshot(output / "menu.ppm")
                host.move_to(70, 65)
                host.run_frames(20)
                host.click()
                host.run_frames(180)
                panel = host.screenshot(output / "panel.ppm")
        require(diff_stats(desktop, menu)[0] > 100, "MAGXDESK menu did not open")
        require(diff_stats(desktop, panel)[0] > 1000, "COPS accessory panel did not open")
        (output / "comparison.txt").write_text("PASS\n")
    except (OSError, RuntimeError) as error:
        print(f"FAIL: {error}")
        return 1
    print("Core ZIP is deterministic, installs byte-for-byte and boots MAGXDESK plus COPS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
