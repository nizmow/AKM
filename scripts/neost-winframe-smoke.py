#!/usr/bin/env python3
"""Compare archived and rebuilt WINFRAME.SLB window rendering on MagiC."""
from __future__ import annotations

import argparse
import os
import sys
import tempfile
from pathlib import Path

from neost_common import acquire_headless, create_output, require, run
from neost_host import NeoSTHost
from neost_image import MagiCImage
from neost_screen import diff_stats

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
ARCHIVE = ROOT / "MagiC/Releases/MAGIC.A/GEMSYS/MAGIC/XTENSION/WINFRAME.SLB"
ARCHIVE_RSC = ROOT / "MagiC/Releases/MAGIC.A/GEMSYS/MAGIC/XTENSION/WINFRAME.RSC"
REBUILT = BUILD / "apps/winframe/WINFRAME.SLB"
REBUILT_RSC = BUILD / "apps/winframe/WINFRAME.RSC"
GUEST_SLB = "::/GEMSYS/MAGIC/XTENSION/WINFRAME.SLB"
GUEST_RSC = "::/GEMSYS/MAGIC/XTENSION/WINFRAME.RSC"


def session(headless: Path, rom: Path, image: MagiCImage, output: Path) -> NeoSTHost:
    host = NeoSTHost(headless, rom, output, image=image,
                     options=["--machine", "megaste", "--mem", "4m",
                              "--no-disk"],
                     auto=r"C:\AUTO\MAGXBOOT.TOS", timeout=240)
    (host.guest_root / "E").mkdir()
    return host


def configure_inf(image: MagiCImage, output: Path, *, disable: bool) -> None:
    inf = output / "MAGX.INF"
    image.copy_out("::/MAGX.INF", inf)
    data = inf.read_bytes()
    data = data.replace(b"#_DEV 1 0", b"#_DEV 2 0")
    require(b"#_DEV 2 0" in data, "MAGX.INF does not select a colour mode")
    if disable:
        data = data.replace(b"#_SLB 0 WINFRAME.SLB", b"; #_SLB 0 WINFRAME.SLB")
        require(b"; #_SLB 0 WINFRAME.SLB" in data,
                "MAGX.INF does not load WINFRAME.SLB")
    inf.write_bytes(data)
    image.copy_in(inf, "::/MAGX.INF")


def run_case(headless: Path, rom: Path, source: Path, slb: Path, rsc: Path,
             output: Path, offset: int, *, disable: bool = False) -> Path:
    output.mkdir()
    image = MagiCImage.clone(source, output / "magic.img", partition_offset=offset)
    image.verify_magic()
    image.copy_in(slb, GUEST_SLB)
    image.copy_in(rsc, GUEST_RSC)
    configure_inf(image, output, disable=disable)

    host = session(headless, rom, image, output / "run")
    try:
        host.start()
        host.boot(15000)
        return host.screenshot(output / "window.ppm")
    finally:
        host.close()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--magic-image", type=Path,
                        default=os.environ.get("MAGIC_IMAGE"))
    parser.add_argument("--tos", type=Path, default=os.environ.get("TOS206_ROM"))
    parser.add_argument("--neost", type=Path, default=Path.home() / ".local/bin/neost")
    parser.add_argument("--neost-headless", type=Path)
    parser.add_argument("--partition-offset", type=int, default=1024)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.magic_image is None or args.tos is None:
        parser.error("supply --magic-image/--tos or MAGIC_IMAGE/TOS206_ROM")
    source = args.magic_image.expanduser().resolve()
    rom = args.tos.expanduser().resolve()
    for path in (source, rom):
        if not path.is_file():
            parser.error(f"input not found: {path}")

    build = run([str(ROOT / "scripts/atari-container"), "run", "make", "-f",
                 "Makefile.apps", "winframe"],
                cwd=ROOT, timeout=900)
    if build.returncode:
        print(build.stdout, file=sys.stderr)
        return build.returncode
    try:
        output = create_output(args.output, parent=BUILD,
                               prefix="neost-winframe").resolve()
        print(f"Evidence: {output}", flush=True)
        with tempfile.TemporaryDirectory(prefix="neost-winframe-") as temporary:
            headless, _ = acquire_headless(args.neost_headless, args.neost,
                                           Path(temporary))
            archived = run_case(headless, rom, source, ARCHIVE, ARCHIVE_RSC,
                                output / "archive", args.partition_offset)
            rebuilt = run_case(headless, rom, source, REBUILT, REBUILT_RSC,
                               output / "rebuilt", args.partition_offset)
            noframe = run_case(headless, rom, source, ARCHIVE, ARCHIVE_RSC,
                               output / "noframe", args.partition_offset, disable=True)
        changed, _ = diff_stats(archived, noframe)
        require(changed > 1000, "WINFRAME did not change the booted window frame")
        count, bbox = diff_stats(archived, rebuilt)
        require(count == 0, f"WINFRAME window differs at {bbox} ({count} pixels)")
        (output / "comparison.txt").write_text("PASS\n")
        print("WINFRAME window frames match the archive", flush=True)
    except (OSError, RuntimeError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
