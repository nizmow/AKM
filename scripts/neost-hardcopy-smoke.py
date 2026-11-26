#!/usr/bin/env python3
"""Compare the archived and rebuilt HARDCOPY extensions on cloned MagiC images.

Each clone runs a small probe from `GEMSYS/MAGIC/START` that reads `scr_dump`,
launches `GEMSYS/MAGIC/HCP/HARDCOPY.PRG`, and reads `scr_dump` again.  The
archived and rebuilt extension must both return status 0 and replace the
kernel's dummy hook; a clone without the extension must leave the hook intact.
NeoST cannot capture this MagiC installation's printer path, so the dump and
printer bytes are not exercised.
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

from neost_common import acquire_headless, create_output, require, run, run_logged
from neost_image import MagiCImage
from neost_screen import diff_stats, read_ppm

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
ARCHIVE = ROOT / "base/hardcopy/HARDCOPY.PRG"
RELEASE = ROOT / "scripts/testdata/release/HARDCOPY.PRG"
REBUILT = BUILD / "apps/hardcopy/HARDCOPY.PRG"
PROBE = BUILD / "apps/hardcopy/HCPROBE.PRG"
LOADER_GUEST = "::/GEMSYS/MAGIC/HCP/HARDCOPY.PRG"
# VASM rewrites seven adda/suba-to-sp instructions as two-byte-shorter lea.
REBUILT_TEXT = 2652
ARCHIVE_TEXT = 2666


def check_header(path: Path, expected_text: int) -> None:
    data = path.read_bytes()
    require(len(data) > 32, f"not a TOS executable: {path}")
    require(data[:2] == b"\x60\x1a", f"missing TOS branch: {path}")
    text, initialized, bss, symbols, _reserved, flags, absolute = \
        struct.unpack_from(">6IH", data, 2)
    require((text, initialized, bss) == (expected_text, 0, 0),
            f"unexpected layout in {path}: {text}/{initialized}/{bss}")
    require(symbols == 0 and absolute == 0,
            f"{path} carries symbols or absolute data")
    require(flags == 0x1, f"{path} flags are {flags:#x}, expected 0x1")
    require(len(data) > 28 + text, f"{path} has no relocation data")


def boot_case(label: str, loader: Path | None, headless: Path, rom: Path,
              source: Path, output: Path, partition_offset: int,
              frames: int) -> tuple[Path, tuple[int, int, int]]:
    case = output / label
    case.mkdir(parents=True)
    image = MagiCImage.clone(source, case / "magic.img",
                             partition_offset=partition_offset)
    image.verify_magic()
    image.copy_in(PROBE, "::/GEMSYS/MAGIC/START/HCPROBE.PRG")
    if loader is None:
        image.mkdir("::/GEMSYS/MAGIC/HCP")
        removed = run(["mdel", "-i", image.image_spec, LOADER_GUEST])
        require(removed.returncode == 0 or "not found" in removed.stdout.lower(),
                f"{label}: could not remove inherited HARDCOPY.PRG")
        absent = run(["mdir", "-i", image.image_spec, LOADER_GUEST])
        require(absent.returncode != 0 and "not found" in absent.stdout.lower(),
                f"{label}: HARDCOPY.PRG is still present")
    else:
        image.mkdir("::/GEMSYS/MAGIC/HCP")
        image.copy_in(loader, LOADER_GUEST)
        image.copy_out(LOADER_GUEST, case / "installed.PRG")
        require(hashlib.sha256((case / "installed.PRG").read_bytes()).hexdigest()
                == hashlib.sha256(loader.read_bytes()).hexdigest(),
                f"{label}: installed extension differs from {loader}")
    removed = run(["mdel", "-i", image.image_spec, "::/HCPROBE.BIN"])
    require(removed.returncode == 0 or "not found" in removed.stdout.lower(),
            f"{label}: could not clear stale HCPROBE.BIN: {removed.stdout.strip()}")
    (case / "GEMDRIVE" / "E").mkdir(parents=True)
    command = [
        str(headless), "--machine", "megaste", "--mem", "4m", "--no-disk",
        "--mono", "--acsi", str(image.path),
        "--gemdos", str(case / "GEMDRIVE"),
        "--auto", r"C:\AUTO\MAGXBOOT.TOS", "--frames", str(frames),
        "--screenshot", str(case / "final.ppm"), str(rom),
    ]
    (case / "command.txt").write_text("\n".join(command) + "\n")
    run_logged(command, case / "headless.log", timeout=180)
    require((case / "final.ppm").is_file(), f"{label}: no screenshot")
    present = run(["mdir", "-i", image.image_spec, "::/HCPROBE.BIN"])
    require(present.returncode == 0,
            f"{label}: probe did not write HCPROBE.BIN: {present.stdout.strip()}")
    image.copy_out("::/HCPROBE.BIN", case / "HCPROBE.BIN")
    record = struct.unpack(">3l", (case / "HCPROBE.BIN").read_bytes())
    return case / "final.ppm", record


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--neost", type=Path, default=Path.home() / ".local/bin/neost")
    parser.add_argument("--neost-headless", type=Path)
    parser.add_argument("--magic-image", type=Path,
                        default=Path(os.environ["MAGIC_IMAGE"]) if "MAGIC_IMAGE" in os.environ else None)
    parser.add_argument("--tos", type=Path,
                        default=Path(os.environ["TOS206_ROM"]) if "TOS206_ROM" in os.environ else None)
    parser.add_argument("--partition-offset", type=int, default=1024)
    parser.add_argument("--frames", type=int, default=12000)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--skip-build", action="store_true")
    args = parser.parse_args()
    if args.magic_image is None or args.tos is None:
        parser.error("pass --magic-image and --tos (or set MAGIC_IMAGE and TOS206_ROM)")
    if args.frames <= 0:
        parser.error("frames must be positive")

    if not args.skip_build:
        build = run([str(ROOT / "scripts/atari-container"), "run", "make", "-f",
                     "Makefile.apps", "hardcopy", "hardcopy-test-driver"],
                    cwd=ROOT, timeout=600)
        if build.returncode:
            print(build.stdout, file=sys.stderr)
            return build.returncode
    for path in (ARCHIVE, RELEASE, REBUILT, PROBE):
        require(path.is_file(), f"missing build or reference file: {path}")
    require(ARCHIVE.read_bytes() == RELEASE.read_bytes(),
            "archived and Release HARDCOPY.PRG differ")
    check_header(ARCHIVE, ARCHIVE_TEXT)
    check_header(REBUILT, REBUILT_TEXT)
    require(REBUILT.read_bytes() != ARCHIVE.read_bytes(),
            "rebuilt extension is an archive copy, not a VASM build")

    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-hardcopy")
    except OSError as error:
        parser.error(str(error))
    try:
        with tempfile.TemporaryDirectory(prefix="neost-hardcopy-") as temporary:
            headless, _ = acquire_headless(args.neost_headless, args.neost, Path(temporary))
            rom = args.tos.expanduser().resolve()
            source = args.magic_image.expanduser().resolve()
            archive_shot, archive_record = boot_case(
                "archive", ARCHIVE, headless, rom, source, output,
                args.partition_offset, args.frames)
            rebuilt_shot, rebuilt_record = boot_case(
                "rebuilt", REBUILT, headless, rom, source, output,
                args.partition_offset, args.frames)
            missing_shot, missing_record = boot_case(
                "missing", None, headless, rom, source, output,
                args.partition_offset, args.frames)
        archive_status, archive_before, archive_after = archive_record
        rebuilt_status, rebuilt_before, rebuilt_after = rebuilt_record
        missing_status, missing_before, missing_after = missing_record
        require(archive_status == 0 and rebuilt_status == 0,
                f"installation failed: archive {archive_status}, rebuilt {rebuilt_status}")
        require(archive_after != archive_before,
                "archived extension did not replace scr_dump")
        require(rebuilt_after != rebuilt_before,
                "rebuilt extension did not replace scr_dump")
        require(missing_status < 0,
                f"missing extension unexpectedly launched with status {missing_status}")
        require(missing_after == missing_before,
                "missing extension changed scr_dump")
        width, height, _ = read_ppm(archive_shot)
        rebuilt_diff, rebuilt_box = diff_stats(archive_shot, rebuilt_shot)
        missing_diff, missing_box = diff_stats(archive_shot, missing_shot)
        require(rebuilt_diff == 0,
                f"archive/rebuilt screens differ at {rebuilt_box} ({rebuilt_diff} pixels)")
        require(missing_diff == 0,
                f"missing-extension screen differs at {missing_box} ({missing_diff} pixels)")
        print(f"HARDCOPY: both builds install the hook; {width}x{height} screens identical; "
              "missing extension launches nothing and leaves scr_dump intact")
        print(f"Evidence: {output}")
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"hardcopy smoke failed: {error}\nArtifacts: {output}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
