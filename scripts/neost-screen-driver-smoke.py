#!/usr/bin/env python3
"""Build the MFA/MFM drivers and compare the Atari screen modes with the archive.

ST high/medium/low select MFA2/MFA4/MFA16.  Replacing a driver with its rebuild
must reach the same desktop; deleting it must reach the loader's missing-driver
halt instead.  MFM (MagiCMac) drivers cannot run on the NeoST Atari profiles.
"""
from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path
import re
import struct
import subprocess
import sys
import tempfile

from neost_common import acquire_headless, create_output, require, run
from neost_image import MagiCImage
from neost_screen import diff_stats, read_ppm

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
ARCHIVE = ROOT / "core/mfa/tests/archive"
MODES = (
    {"label": "high", "dev": b"1 0", "mono": True, "driver": "MFA2"},
    {"label": "medium", "dev": b"3 0", "mono": False, "driver": "MFA4"},
    {"label": "low", "dev": b"2 0", "mono": False, "driver": "MFA16"},
)
DRIVERS = {
    "MFA2": (0x0313, (2, 1, 2, 1)),
    "MFA4": (0x0313, (4, 2, 0, 1)),
    "MFA16": (0x0313, (16, 4, 0, 1)),
    "MFA256": (0x0313, (256, 8, 0, 1)),
    "MFA32K": (0x0313, (32768, 16, 2, 2)),
    "MFM2": (0x0313, (2, 1, 2, 1)),
    "MFM16": (0x0313, (16, 4, 2, 1)),
    "MFM16IP": (0x0313, (16, 4, 0, 1)),
    "MFM16M": (0x0313, (16777216, 32, 2, 1)),
    "MFM256": (0x0313, (256, 8, 2, 1)),
    "MFM32K": (0x0313, (32768, 16, 2, 1)),
    "MFM4IP": (0x0314, (4, 2, 0, 1)),
}


def check_driver(path: Path, expected_version: int, expected_org: tuple) -> dict:
    data = path.read_bytes()
    require(len(data) >= 0x60, f"not a TOS executable: {path}")
    tlen, dlen, blen, syms = struct.unpack_from(">IIII", data, 2)
    require(data[:2] == b"\x60\x1a", f"missing TOS branch: {path}")
    require(data[28:38] == b"\x60\x4eNVDIDRV\x00", f"missing NVDIDRV header: {path}")
    version, header, kind = struct.unpack_from(">HHH", data, 38)
    require((header, kind) == (80, 1), f"unexpected driver header: {path}")
    require(version == expected_version,
            f"unexpected version {version:04x}: {path}")
    require(struct.unpack_from(">IHHH", data, 92) == expected_org,
            f"unexpected organization: {path}")
    end = 28 + tlen + dlen + syms
    require(end + 4 <= len(data), f"segments exceed file: {path}")
    position = struct.unpack_from(">I", data, end)[0]
    require(position and position % 2 == 0 and position <= tlen + dlen,
            f"invalid relocation table offset: {path}")
    relocations = {position}
    cursor = end + 4
    while cursor < len(data) and data[cursor]:
        delta = data[cursor]
        cursor += 1
        position += 254 if delta == 1 else delta
        require(position % 2 == 0 and position + 4 <= tlen + dlen,
                f"relocation outside text/data: {path}")
        relocations.add(position)
    require(cursor + 1 == len(data), f"trailing relocation bytes: {path}")
    require(16 in relocations and 20 in relocations,
            f"driver entry slots are not relocated: {path}")
    for slot in (16, 20):
        entry = struct.unpack_from(">I", data, 28 + slot)[0]
        require(80 <= entry < tlen and entry % 2 == 0,
                f"invalid relocated entry at text+{slot:#x}: {path}")
    return {"length": len(data), "tlen": tlen, "dlen": dlen, "blen": blen,
            "version": version, "relocations": len(relocations),
            "sha256": hashlib.sha256(data).hexdigest()}


def guest_present(image: MagiCImage, guest: str) -> bool:
    result = run(["mdir", "-i", image.image_spec, guest])
    return result.returncode == 0


def boot_case(label: str, driver: Path | None, mode: dict, headless: Path,
              rom: Path, source: Path, output: Path, partition_offset: int,
              frames: int) -> tuple[Path, bool]:
    case = output / mode["label"] / label
    case.mkdir(parents=True)
    image = MagiCImage.clone(source, case / "magic.img",
                             partition_offset=partition_offset)
    image.verify_magic()
    inf = case / "MAGX.INF"
    image.copy_out("::/MAGX.INF", inf)
    text = inf.read_bytes()
    pattern = re.compile(rb"(?im)^([ \t]*#_DEV[ \t]+)\d+[ \t]+\d+")
    require(len(pattern.findall(text)) == 1,
            f"{mode['label']}: expected one active #_DEV setting")
    text = pattern.sub(lambda match: match.group(1) + mode["dev"],
                       text, count=1)
    inf.write_bytes(text)
    image.copy_in(inf, "::/MAGX.INF")
    guest = f"::/GEMSYS/{mode['driver']}.SYS"
    if driver is None:
        result = run(["mdel", "-i", image.image_spec, guest])
        require(result.returncode == 0, f"{label}: could not remove {guest}")
    else:
        image.copy_in(driver, guest)
    image.copy_in(BUILD / "apps/screen/SCREEN.PRG",
                  "::/GEMSYS/MAGIC/START/SCREEN.PRG")
    run(["mdel", "-i", image.image_spec, "::/SCREEN.TXT"])
    (case / "GEMDRIVE" / "E").mkdir(parents=True)
    command = [
        str(headless), "--machine", "megaste", "--mem", "4m", "--no-disk",
        "--acsi", str(image.path), "--gemdos", str(case / "GEMDRIVE"),
        "--auto", r"C:\AUTO\MAGXBOOT.TOS", "--frames", str(frames),
        "--screenshot", str(case / "final.ppm"), str(rom),
    ]
    if mode["mono"]:
        command.insert(6, "--mono")
    result = run(command, timeout=120)
    require(result.returncode == 0, f"{label}: NeoST failed: {result.stdout}")
    require((case / "final.ppm").is_file(), f"{label}: no screenshot")
    installed = False
    if driver is not None:
        image.copy_out(guest, case / "installed.SYS")
        installed = True
        require(hashlib.sha256((case / "installed.SYS").read_bytes()).hexdigest()
                == hashlib.sha256(driver.read_bytes()).hexdigest(),
                f"{label}: installed driver differs from {driver}")
    marker = None
    if guest_present(image, "::/SCREEN.TXT"):
        image.copy_out("::/SCREEN.TXT", case / "SCREEN.TXT")
        marker = (case / "SCREEN.TXT").read_bytes()
    return case / "final.ppm", installed, marker


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--neost", type=Path, default=Path.home() / ".local/bin/neost")
    parser.add_argument("--neost-headless", type=Path)
    parser.add_argument("--magic-image", type=Path,
                        default=Path(os.environ["MAGIC_IMAGE"]) if "MAGIC_IMAGE" in os.environ else None)
    parser.add_argument("--tos", type=Path,
                        default=Path(os.environ["TOS206_ROM"]) if "TOS206_ROM" in os.environ else None)
    parser.add_argument("--partition-offset", type=int, default=1024)
    parser.add_argument("--frames", type=int, default=8000)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.magic_image is None or args.tos is None:
        parser.error("pass --magic-image and --tos (or set MAGIC_IMAGE and TOS206_ROM)")

    build = subprocess.run(
        [str(ROOT / "scripts/atari-container"), "run", "make", "-f", "Makefile.apps",
         "screen-drivers", "screen-boot-probe"], cwd=ROOT, text=True,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=600)
    if build.returncode:
        print(build.stdout, file=sys.stderr)
        return build.returncode
    require((BUILD / "apps/screen/SCREEN.PRG").is_file(),
            "build did not produce the screen boot probe")
    infos = {}
    for name, (version, org) in DRIVERS.items():
        path = BUILD / "apps/screen" / (name + ".SYS")
        require(path.is_file(), f"build did not produce {path}")
        infos[name] = check_driver(path, version, org)
    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-screen")
    except OSError as error:
        parser.error(str(error))
    print(f"screen drivers: {len(infos)} structurally checked")

    try:
        with tempfile.TemporaryDirectory(prefix="neost-screen-") as temporary:
            headless, _ = acquire_headless(args.neost_headless, args.neost, Path(temporary))
            rom = args.tos.expanduser().resolve()
            source = args.magic_image.expanduser().resolve()
            for mode in MODES:
                name = mode["driver"]
                archive = ARCHIVE / (name + ".SYS")
                rebuilt = BUILD / "apps/screen" / (name + ".SYS")
                check_driver(archive, *DRIVERS[name])
                archive_shot, archive_installed, archive_marker = boot_case(
                    "archive", archive, mode, headless, rom, source, output,
                    args.partition_offset, args.frames)
                rebuilt_shot, rebuilt_installed, rebuilt_marker = boot_case(
                    "rebuilt", rebuilt, mode, headless, rom, source, output,
                    args.partition_offset, args.frames)
                missing_shot, missing_installed, missing_marker = boot_case(
                    "missing", None, mode, headless, rom, source, output,
                    args.partition_offset, args.frames)
                require(archive_installed and rebuilt_installed and not missing_installed,
                        f"{name}: driver installation state is wrong")
                require(archive_marker and archive_marker.startswith(b"MAGIC") and
                        rebuilt_marker and rebuilt_marker.startswith(b"MAGIC"),
                        f"{name}: MagiC did not reach the boot probe")
                require(missing_marker is None,
                        f"{name}: missing driver still reached the boot probe")
                width, height, _ = read_ppm(archive_shot)
                rebuilt_diff, rebuilt_box = diff_stats(archive_shot, rebuilt_shot)
                missing_diff, missing_box = diff_stats(archive_shot, missing_shot)
                require(rebuilt_diff == 0,
                        f"{name}: archive/rebuilt screens differ at {rebuilt_box} "
                        f"({rebuilt_diff} pixels)")
                require(missing_diff > width * height // 10,
                        f"{name}: missing-driver screen is not distinct ({missing_diff} pixels)")
                print(f"{name} ({mode['label']}): identical {width}x{height} screen; "
                      f"missing driver differs by {missing_diff} pixels at {missing_box}")
        print(f"Evidence: {output}")
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"screen-driver smoke failed: {error}\nArtifacts: {output}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
