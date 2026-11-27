#!/usr/bin/env python3
"""Compare archived and rebuilt MAGXCONF.CPX boot init and dialog on MagiC."""
from __future__ import annotations

import argparse
import os
import struct
import sys
import tempfile
from pathlib import Path

COMPONENT = Path(__file__).resolve().parents[1]
ROOT = COMPONENT.parents[1]
sys.path.insert(0, str(ROOT / "scripts"))

from neost_common import acquire_headless, create_output, require, run
from neost_host import NeoSTHost
from neost_image import MagiCImage
from neost_screen import diff_stats

BUILD = ROOT / "build/atari"
APPS = BUILD / "apps"
ARCHIVE = COMPONENT / "tests/MAGXCONF.CPX"
REBUILT = APPS / "cpx/MAGXCONF.CPX"
PROBE = APPS / "magxconf/NEOSTART.PRG"
GUEST_CPX = "::/AUTO/ACCS/CPX/MAGXCONF.CPX"
GUEST_CPZ = "::/AUTO/ACCS/CPX/MAGXCONF.CPZ"
EXPECTED = 0x10


def saved_default(cpx: Path, destination: Path, value: int) -> Path:
    """Place a known saved default in a scratch CPX copy."""
    data = bytearray(cpx.read_bytes())
    tlen = struct.unpack_from(">I", data, 512 + 2)[0]
    offset = 512 + 28 + tlen
    struct.pack_into(">I", data, offset, value)
    destination.write_bytes(data)
    return destination


def session(headless: Path, rom: Path, image: MagiCImage, output: Path) -> NeoSTHost:
    host = NeoSTHost(headless, rom, output, image=image,
                     options=["--machine", "megaste", "--mem", "4m", "--mono",
                              "--no-disk"],
                     auto=r"C:\AUTO\MAGXBOOT.TOS", timeout=240)
    (host.guest_root / "E").mkdir()
    return host


def run_case(headless: Path, rom: Path, source: Path, cpx: Path, output: Path,
             offset: int) -> Path:
    output.mkdir()
    image = MagiCImage.clone(source, output / "magic.img", partition_offset=offset)
    image.verify_magic()
    if run(["mdir", "-i", image.image_spec, GUEST_CPZ]).returncode == 0:
        moved = run(["mren", "-o", "-i", image.image_spec, GUEST_CPZ, GUEST_CPX])
        require(moved.returncode == 0, f"cannot enable CPX: {moved.stdout}")
    patched = saved_default(cpx, output / "test.cpx", EXPECTED)
    image.copy_in(patched, GUEST_CPX)
    image.copy_in(PROBE, "::/GEMSYS/MAGIC/START/ZCPXTEST.PRG")
    (output / "CNFMODE.TXT").write_text("r")
    image.copy_in(output / "CNFMODE.TXT", "::/CNFMODE.TXT")

    host = session(headless, rom, image, output / "run")
    with host:
        host.boot(15000)
        host.calibrate_mouse()
        result = host.guest_root / "E/BEFORE.RES"
        require(result.is_file(), f"no boot probe result: {result}")
        value = int(result.read_text(encoding="ascii").split()[0], 16)
        require(value == EXPECTED, f"boot initializer set {value:08x}, expected {EXPECTED:08x}")
        host.move_to(40, 8)
        host.run_frames(30)
        host.click(40, 8)
        host.run_frames(60)
        host.move_to(70, 65)
        host.run_frames(20)
        host.click(70, 65)
        host.run_frames(180)
        host.move_to(205, 195)
        host.run_frames(20)
        host.click(205, 195)
        host.click(205, 195)
        host.run_frames(150)
        return host.screenshot("dialog.ppm")


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
                 "Makefile.apps", "magxconf-cpx", "magxconf-test-driver"],
                cwd=ROOT, timeout=900)
    if build.returncode:
        print(build.stdout, file=sys.stderr)
        return build.returncode
    try:
        output = create_output(args.output, parent=BUILD,
                               prefix="neost-magxconf-cpx").resolve()
        print(f"Evidence: {output}", flush=True)
        with tempfile.TemporaryDirectory(prefix="neost-magxconf-cpx-") as temporary:
            headless, _ = acquire_headless(args.neost_headless, args.neost,
                                           Path(temporary))
            archived = run_case(headless, rom, source, ARCHIVE,
                                output / "archive", args.partition_offset)
            rebuilt = run_case(headless, rom, source, REBUILT,
                               output / "rebuilt", args.partition_offset)
        count, bbox = diff_stats(archived, rebuilt)
        require(count == 0, f"CPX dialog differs at {bbox} ({count} pixels)")
        (output / "comparison.txt").write_text("PASS\n")
        print("boot initializer and CPX dialog match the archive", flush=True)
    except (OSError, RuntimeError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
