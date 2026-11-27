#!/usr/bin/env python3
"""Build, install and cold-boot an Atari/ST release profile."""
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
PACKAGE = ROOT / "scripts/make-release.py"
PAYLOAD = ("AUTO", "BIN", "EXTRAS", "GEMSYS", "LANG")
ROOT_FILES = ("MAGX.INF", "MAGIC.RAM", "AUTOEXEC.BAT", "INSTALL.TXT", "NEWDESK.INF")
DESKTOP_APP = "::/GEMSYS/GEMDESK/MAGXDESK.APP"


def install(source: Path, folder: Path, offset: int, package: Path) -> MagiCImage:
    image = MagiCImage.clone(source, folder / "magic.img", partition_offset=offset)
    image.verify_magic()
    for name in PAYLOAD:
        image.remove(f"::/{name}", tree=True)
    for name in ROOT_FILES:
        image.remove(f"::/{name}")
    extracted = folder / "extract"
    with zipfile.ZipFile(package) as archive:
        archive.extractall(extracted)
    result = run(["mcopy", "-s", "-o", "-i", image.image_spec,
                  *map(str, sorted(extracted.iterdir())), "::/"], timeout=300)
    require(result.returncode == 0, f"clean extraction failed: {result.stdout}")
    return image


def verify_members(image: MagiCImage, folder: Path, package: Path) -> None:
    restored = folder / "verify"
    with zipfile.ZipFile(package) as archive:
        members = sorted(archive.namelist())
        for name in members:
            target = restored / name
            target.parent.mkdir(parents=True, exist_ok=True)
            image.copy_out(f"::/{name}", target)
            expected = hashlib.sha256(archive.read(name)).hexdigest()
            require(sha256(target) == expected, f"installed member differs from ZIP: {name}")


def boot(headless: Path, rom: Path, folder: Path, image: MagiCImage, *,
         open_menu: bool, open_cops: bool = False) -> Path:
    host = NeoSTHost(headless, rom, folder / "run", image=image,
                     options=["--machine", "megaste", "--mem", "4m", "--mono", "--no-disk"],
                     auto=r"C:\AUTO\MAGXBOOT.TOS", timeout=240)
    with host:
        host.boot(15000)
        host.calibrate_mouse()
        host.move_to(408, 384)
        host.run_frames(10)
        desktop = host.screenshot(folder / "desktop.ppm")
        if open_menu:
            host.move_to(40, 8)
            host.run_frames(30)
            host.click()
            host.run_frames(60)
            menu = host.screenshot(folder / "menu.ppm")
            require(diff_stats(desktop, menu)[0] > 100, "desktop menu did not open")
        if open_cops:
            host.move_to(70, 65)
            host.run_frames(20)
            host.click()
            host.run_frames(180)
            panel = host.screenshot(folder / "panel.ppm")
            require(diff_stats(desktop, panel)[0] > 1000, "COPS accessory panel did not open")
    return desktop


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--magic-image", type=Path, default=os.environ.get("MAGIC_IMAGE"))
    parser.add_argument("--tos", type=Path, default=os.environ.get("TOS206_ROM"))
    parser.add_argument("--neost", type=Path, default=Path.home() / ".local/bin/neost")
    parser.add_argument("--neost-headless", type=Path)
    parser.add_argument("--partition-offset", type=int, default=1024)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--profile", choices=("core", "minimal"), default="core")
    parser.add_argument("--language", choices=("de", "en"), default="de")
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--skip-build", action="store_true")
    args = parser.parse_args()
    if args.magic_image is None or args.tos is None:
        parser.error("supply --magic-image/--tos or MAGIC_IMAGE/TOS206_ROM")
    source, rom = (path.expanduser().resolve() for path in (args.magic_image, args.tos))
    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-release").resolve()
        print(f"Evidence: {output}", flush=True)
        stem = f"{args.profile}-{args.language}"
        package = output / f"{stem}.zip"
        repeat = output / f"{stem}-repeat.zip"
        manifest = output / f"{stem}.manifest"
        command = [sys.executable, str(PACKAGE), "--profile", args.profile,
                   "--language", args.language]
        if args.build_dir is not None:
            command += ["--build-dir", str(args.build_dir)]
        result = run([*command, "--output", str(package), "--manifest", str(manifest),
                      *(["--skip-build"] if args.skip_build else [])], cwd=ROOT, timeout=1200)
        require(result.returncode == 0, result.stdout)
        result = run([*command, "--output", str(repeat), "--skip-build"], cwd=ROOT, timeout=300)
        require(result.returncode == 0, result.stdout)
        require(sha256(package) == sha256(repeat), "two package runs produced different ZIPs")
        image = install(source, output, args.partition_offset, package)
        verify_members(image, output, package)
        minimal = args.profile == "minimal"
        if minimal:
            optional = ["::/AUTO/ACCS", "::/GEMSYS/MAGIC/START"]
            if args.language == "de":
                optional.append("::/GEMSYS/MAGIC/XTENSION")
            else:
                with zipfile.ZipFile(package) as archive:
                    extensions = {name for name in archive.namelist()
                                  if name.startswith("GEMSYS/MAGIC/XTENSION/")}
                require(extensions == {"GEMSYS/MAGIC/XTENSION/KEYTABLS.SYS"},
                        f"unexpected minimal extension payload: {extensions}")
            for guest in optional:
                require(not image.exists(guest), f"unexpected optional payload: {guest}")
            control_dir = output / "missing-desktop"
            control_dir.mkdir()
            control = MagiCImage.clone(image.path, control_dir / "magic.img",
                                       partition_offset=args.partition_offset)
            control.remove(DESKTOP_APP)
        with tempfile.TemporaryDirectory(prefix="neost-release-") as scratch:
            headless, _ = acquire_headless(args.neost_headless, args.neost, Path(scratch))
            desktop = boot(headless, rom, output, image, open_menu=True, open_cops=not minimal)
            if minimal:
                missing = boot(headless, rom, control_dir, control, open_menu=False)
                require(diff_stats(desktop, missing)[0] > 1000,
                        "desktop indistinguishable from missing-MAGXDESK control")
        (output / "comparison.txt").write_text("PASS\n")
    except (OSError, RuntimeError) as error:
        print(f"FAIL: {error}")
        return 1
    desktop_name = "MAGXDESK alone" if minimal else "MAGXDESK plus COPS"
    print(f"{args.language} {args.profile} ZIP is deterministic, installs byte-for-byte and boots {desktop_name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
