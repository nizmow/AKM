#!/usr/bin/env python3
"""Compare rebuilt image SLBs with a selected legacy baseline under NeoST.

Each pair runs the same GCC probe against a fresh image clone and the same
PAT6 fixture. The probe checks the known XP conversion, then exercises
LOAD_IMG exports #0, #2, #3 and #1 and hashes rendered colours rather than
descriptor or framebuffer storage.
"""
from __future__ import annotations

import argparse
import os
from pathlib import Path
import sys
import tempfile

from neost_common import acquire_headless, create_output, require, run
from neost_image import MagiCImage
from neost_host import NeoSTHost

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
ARCHIVE = ROOT / "MagiC/Releases/MAGIC.A/GEMSYS/MAGIC/XTENSION"
ARCHIVE_XP = ROOT / "MagiC/EXTENSIO/XP_RASTR.SLB/BIN/XP_RASTR.SLB"
ARCHIVE_LOAD = ARCHIVE / "LOAD_IMG.SLB"
SOURCE_LOAD = ROOT / "MagiC/TOOLS/LOAD_IMG.SLB/BIN/LOAD_IMG.SLB"
PAT6 = ROOT / "MagiC/Releases/MAGIC.A/GEMSYS/GEMDESK/PAT/16/PAT6.IMG"
XP = BUILD / "apps/xp-rastr/XP_RASTR.SLB"
LOAD = BUILD / "apps/load-img/LOAD_IMG.SLB"
PROBE = BUILD / "apps/slbtest/SLBTEST.PRG"


def run_probe(name: str, pair: tuple[Path, Path], source: Path,
              output: Path, headless: Path, rom: Path) -> str:
    directory = output / name
    directory.mkdir()
    image = MagiCImage.clone(source, directory / "magic.img", partition_offset=1024)
    image.verify_magic()
    image.install(((pair[0], "::/GEMSYS/MAGIC/XTENSION/XP_RASTR.SLB"),
                   (pair[1], "::/GEMSYS/MAGIC/XTENSION/LOAD_IMG.SLB"),
                   (PAT6, "::/GEMSYS/GEMDESK/PAT/16/PAT6.IMG"),
                   (PROBE, "::/GEMSYS/MAGIC/START/ZCNFTEST.PRG")))
    # Never accept a marker left in the input image as proof of execution.
    run(["mdel", "-i", image.image_spec, "::/SLBTEST.TXT"], timeout=30)
    stale = directory / "stale-marker.txt"
    stale_check = run(["mcopy", "-i", image.image_spec, "::/SLBTEST.TXT",
                       str(stale)], timeout=30)
    require(stale_check.returncode != 0,
            f"{name}: stale SLBTEST.TXT marker survived removal")
    inf = directory / "MAGX.INF"
    image.copy_out("::/MAGX.INF", inf)
    inf.write_bytes(inf.read_bytes().replace(b"#_DEV 1 0", b"#_DEV 2 0"))
    image.copy_in(inf, "::/MAGX.INF")

    host = NeoSTHost(headless, rom, directory / "run", image=image,
                     options=["--machine", "megaste", "--mem", "4m", "--no-disk"],
                     auto=r"C:\AUTO\MAGXBOOT.TOS", timeout=240)
    try:
        host.start()
        host.boot(15000)
        host.screenshot(directory / "screen.ppm")
        result = directory / "SLBTEST.TXT"
        image.copy_out("::/SLBTEST.TXT", result)
        require(result.is_file(), f"{name}: probe did not write SLBTEST.TXT")
        text = result.read_text(errors="replace").strip()
        print(f"{name} SLB consumer result: {text}")
        return text
    finally:
        host.close()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--magic-image", type=Path,
                        default=os.environ.get("MAGIC_IMAGE"))
    parser.add_argument("--tos", type=Path, default=os.environ.get("TOS206_ROM"))
    parser.add_argument("--neost-headless", type=Path)
    parser.add_argument("--baseline", choices=("source", "release"), default="source",
                        help="compare with the source-tree archive or the shipped Release")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.magic_image is None or args.tos is None:
        parser.error("supply --magic-image/--tos or MAGIC_IMAGE/TOS206_ROM")
    source, rom = args.magic_image.expanduser().resolve(), args.tos.expanduser().resolve()
    archive_load = SOURCE_LOAD if args.baseline == "source" else ARCHIVE_LOAD
    for path in (source, rom, ARCHIVE_XP, archive_load, PAT6):
        require(path.is_file(), f"required input not found: {path}")

    build = run(["scripts/atari-container", "run", "make", "-f", "Makefile.apps",
                 "xp-rastr", "load-img", "slbtest"], cwd=ROOT, timeout=300)
    if build.returncode:
        print(build.stdout, file=sys.stderr)
        print(f"ERROR: SLB consumer build exited {build.returncode}", file=sys.stderr)
        return build.returncode
    for path in (XP, LOAD, PROBE):
        require(path.is_file(), f"build completed without producing {path}")

    output = create_output(args.output, parent=BUILD, prefix="neost-slb-consumer")
    print(f"Evidence: {output}", flush=True)
    with tempfile.TemporaryDirectory(prefix="neost-slb-") as temporary:
        headless, _ = acquire_headless(args.neost_headless,
                                       Path.home() / ".local/bin/neost",
                                       Path(temporary))
        if args.baseline == "source":
            # Slbopen needs space for its own bookkeeping after Pexec. The
            # source archive predates the minimal-allocation flag used by
            # the shipped SLB. Adjust only the flag in a disposable copy.
            archive_data = bytearray(archive_load.read_bytes())
            require(archive_data[22:26] == b"\x00\x00\x00\x07",
                    "unexpected source archive TOS program flags")
            archive_data[25] |= 8
            archive_load = output / SOURCE_LOAD.name
            archive_load.write_bytes(archive_data)
        archived = run_probe(args.baseline, (ARCHIVE_XP, archive_load), source,
                             output, headless, rom)
        rebuilt = run_probe("rebuilt", (XP, LOAD), source, output, headless, rom)
    expected_suffix = "72246d45"
    for name, text in ((args.baseline, archived), ("rebuilt", rebuilt)):
        require(text.startswith("ok 64 64 ") and text.endswith(expected_suffix),
                f"{name}: probe failed or XP conversion differed: {text}")
    (output / "comparison.txt").write_text(
        f"{args.baseline}={archived}\nrebuilt={rebuilt}\n", encoding="utf-8")
    require(archived == rebuilt,
            f"rendered IMG colours differ from {args.baseline}: {archived} != {rebuilt}")
    print(f"{args.baseline} and rebuilt IMG colours: identical")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        raise SystemExit(1)
