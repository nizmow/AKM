#!/usr/bin/env python3
"""Compare the shipped and rebuilt MAGXBOOT loaders on cloned MagiC images.

The loader starts `MAGIC.RAM`; it does not modify it.  Each case runs on its
own clone with a `GEMSYS/MAGIC/START` probe that writes `C:\\SCREEN.TXT` once
MagiC is running.  Replacing the loader must reach the marker and MAGXDESK;
deleting it must not reach either.  The rebuilt loader differs from the archive
only in two `adda.w #12,sp` encodings that VASM rewrites to `lea 12(sp),sp`.
"""
from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile

COMPONENT = Path(__file__).resolve().parents[1]
ROOT = COMPONENT.parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from neost_common import acquire_headless, create_output, require, run, run_logged
from neost_image import MagiCImage
from neost_screen import diff_stats, read_ppm

BUILD = ROOT / "build/atari"
ARCHIVE = ROOT / "core/magxboot/tests/MAGXBOOT.TOS"
ARCHIVED_PRG = COMPONENT / "SRC/MAGXBOOT.PRG"
REBUILT = BUILD / "apps/magxboot/MAGXBOOT.TOS"
PROBE = BUILD / "apps/screen/SCREEN.PRG"
LOADER_GUEST = "::/AUTO/MAGXBOOT.TOS"
MARKER = b"MAGIC\r\n"
# TEXT length identifies the variant; DATA/SYMBOLS are empty in every loader.
VARIANTS = {"MAGXBOOT.PRG": 942, "MAGXBO32.PRG": 1016}


def check_loader(path: Path, expected_text: int) -> None:
    data = path.read_bytes()
    require(len(data) >= 32, f"not a TOS executable: {path}")
    require(data[:2] == b"\x60\x1a", f"missing TOS branch: {path}")
    text, initialized, bss, symbols, _reserved, flags, absolute = \
        struct.unpack_from(">6IH", data, 2)
    require((text, initialized, bss) == (expected_text, 0, 800),
            f"unexpected loader layout in {path}: {text}/{initialized}/{bss}")
    require(symbols == 0 and absolute == 0,
            f"loader {path} carries symbols or absolute data")
    require(flags == 0x7, f"loader {path} flags are {flags:#x}, expected 0x7")


def check_rebuild(rebuilt: Path, archive: Path) -> None:
    """Require the VASM output signature, not an archive copy or stale build."""
    new = rebuilt.read_bytes()
    old = archive.read_bytes()
    require(len(new) == len(old),
            f"{rebuilt.name}: {len(new)} bytes, archive has {len(old)}")
    changed = [i for i, (a, b) in enumerate(zip(new, old)) if a != b]
    require(changed == [64, 65, 282, 283],
            f"{rebuilt.name}: unexpected rebuild deltas at {changed}")
    for offset in (64, 282):
        require(new[offset:offset + 2] == b"\x4f\xef"
                and old[offset:offset + 2] == b"\xde\xfc",
                f"{rebuilt.name}: unexpected encoding at {offset}")


def boot_case(label: str, loader: Path | None, headless: Path, rom: Path,
              source: Path, output: Path, partition_offset: int,
              frames: int) -> tuple[Path, bytes | None]:
    case = output / label
    case.mkdir(parents=True)
    image = MagiCImage.clone(source, case / "magic.img",
                             partition_offset=partition_offset)
    image.verify_magic()
    image.copy_in(PROBE, "::/GEMSYS/MAGIC/START/SCREEN.PRG")
    if loader is None:
        result = run(["mdel", "-i", image.image_spec, LOADER_GUEST])
        require(result.returncode == 0, f"{label}: could not remove the loader")
    else:
        image.copy_in(loader, LOADER_GUEST)
        image.copy_out(LOADER_GUEST, case / "installed.TOS")
        require(hashlib.sha256((case / "installed.TOS").read_bytes()).hexdigest()
                == hashlib.sha256(loader.read_bytes()).hexdigest(),
                f"{label}: installed loader differs from {loader}")
    image.copy_out("::/MAGIC.RAM", case / "MAGIC.RAM")
    run(["mdel", "-i", image.image_spec, "::/SCREEN.TXT"])
    (case / "GEMDRIVE" / "E").mkdir(parents=True)
    command = [
        str(headless), "--machine", "megaste", "--mem", "4m", "--no-disk",
        "--mono", "--acsi", str(image.path),
        "--gemdos", str(case / "GEMDRIVE"),
        "--auto", r"C:\AUTO\MAGXBOOT.TOS", "--frames", str(frames),
        "--screenshot", str(case / "final.ppm"), str(rom),
    ]
    (case / "command.txt").write_text("\n".join(command) + "\n")
    run_logged(command, case / "headless.log", timeout=120)
    require((case / "final.ppm").is_file(), f"{label}: no screenshot")
    marker = None
    present = run(["mdir", "-i", image.image_spec, "::/SCREEN.TXT"])
    if present.returncode == 0:
        image.copy_out("::/SCREEN.TXT", case / "SCREEN.TXT")
        marker = (case / "SCREEN.TXT").read_bytes()
    elif "not found" not in present.stdout.lower():
        raise RuntimeError(
            f"{label}: could not inspect C:\\SCREEN.TXT: {present.stdout.strip()}")
    return case / "final.ppm", marker


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
    parser.add_argument("--skip-build", action="store_true")
    args = parser.parse_args()
    if args.magic_image is None or args.tos is None:
        parser.error("pass --magic-image and --tos (or set MAGIC_IMAGE and TOS206_ROM)")
    if args.frames <= 0:
        parser.error("frames must be positive")

    if not args.skip_build:
        build = run([str(ROOT / "scripts/atari-container"), "run", "make", "-f",
                     "Makefile.apps", "magxboot", "screen-boot-probe"],
                    cwd=ROOT, timeout=600)
        if build.returncode:
            print(build.stdout, file=sys.stderr)
            return build.returncode
    require(PROBE.is_file(), "build did not produce the screen boot probe")
    check_loader(ARCHIVE, VARIANTS["MAGXBOOT.PRG"])
    check_loader(ARCHIVED_PRG, VARIANTS["MAGXBOOT.PRG"])
    require(ARCHIVE.read_bytes() == ARCHIVED_PRG.read_bytes(),
            "shipped loader and archived MAGXBOOT.PRG differ")
    for name, text in VARIANTS.items():
        check_loader(BUILD / "apps/magxboot" / name, text)
    check_loader(REBUILT, VARIANTS["MAGXBOOT.PRG"])
    require((BUILD / "apps/magxboot/MAGXBOOT.PRG").read_bytes()
            == REBUILT.read_bytes(),
            "normal PRG output is not the rebuilt TOS")
    check_rebuild(REBUILT, ARCHIVE)
    check_rebuild(BUILD / "apps/magxboot/MAGXBO32.PRG",
                  COMPONENT / "SRC/MAGXBO32.PRG")

    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-magxboot")
    except OSError as error:
        parser.error(str(error))
    try:
        with tempfile.TemporaryDirectory(prefix="neost-magxboot-") as temporary:
            headless, _ = acquire_headless(args.neost_headless, args.neost, Path(temporary))
            rom = args.tos.expanduser().resolve()
            source = args.magic_image.expanduser().resolve()
            archive_shot, archive_marker = boot_case(
                "archive", ARCHIVE, headless, rom, source, output,
                args.partition_offset, args.frames)
            rebuilt_shot, rebuilt_marker = boot_case(
                "rebuilt", REBUILT, headless, rom, source, output,
                args.partition_offset, args.frames)
            missing_shot, missing_marker = boot_case(
                "missing", None, headless, rom, source, output,
                args.partition_offset, args.frames)
        require(archive_marker == MARKER,
                f"archive loader did not reach the probe: {archive_marker!r}")
        require(rebuilt_marker == MARKER,
                f"rebuilt loader did not reach the probe: {rebuilt_marker!r}")
        require(missing_marker is None,
                "missing loader still reached the boot probe")
        width, height, _ = read_ppm(archive_shot)
        rebuilt_diff, rebuilt_box = diff_stats(archive_shot, rebuilt_shot)
        missing_diff, missing_box = diff_stats(archive_shot, missing_shot)
        require(rebuilt_diff == 0,
                f"archive/rebuilt screens differ at {rebuilt_box} ({rebuilt_diff} pixels)")
        require(missing_diff > width * height // 10,
                f"missing-loader screen is not distinct ({missing_diff} pixels)")
        print(f"loader: identical {width}x{height} MAGXDESK screen; "
              f"missing loader differs by {missing_diff} pixels at {missing_box}")
        print(f"Evidence: {output}")
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"magxboot smoke failed: {error}\nArtifacts: {output}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
