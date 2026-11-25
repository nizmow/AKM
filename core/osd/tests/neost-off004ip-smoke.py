#!/usr/bin/env python3
"""Build OFF004IP.OSD and prove MagiC/NVDI's 2-plane offscreen path.

The rebuild must reproduce the archived V4.10 MIN-ATARI driver exactly.  The
runtime probe installs archive and rebuild as C:\\GEMSYS\\OFF004IP.OSD in
separate MagiC clones, opens a caller-owned 64x64, two-plane interleaved MFDB
through v_opnbm(), fills two bars with VDI pens 2 and 3, reads five pixels back
and dumps the 1024-byte bitmap.  Pen mapping is {0,3,1,2}, so the two pens
exercise each plane independently, and the samples straddle the x=15/16 word
boundary to catch a wrong interleaved stride.
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

sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "scripts"))

from neost_common import acquire_headless, create_output, require
from neost_image import MagiCImage
from neost_server import NeoSTServer

ROOT = Path(__file__).resolve().parents[3]
BUILD = ROOT / "build/atari"
REBUILT = BUILD / "apps/osd/OFF004IP.OSD"
PROBE = BUILD / "apps/off004ip/OFF004IP.PRG"
ARCHIVE = ROOT / "MagiC/KERNEL/VDI/DRIVERS/OSD/MIN-ATAR/OFF004IP.OSD"
RELEASE = ROOT / "MagiC/Releases/MAGIC.A/GEMSYS/OFF004IP.OSD"

# pel is the raw bitmap colour; index is the VDI pen (color_to_vdi).
EXPECTED_PIXELS = ["1/2", "1/2", "2/3", "2/3", "0/0"]


def parse_tos(path: Path) -> dict:
    data = path.read_bytes()
    require(len(data) >= 0x20, f"not a TOS executable: {path}")
    tlen, dlen, blen, syms = struct.unpack(">IIII", data[2:18])
    text = data[0x1c:0x1c + tlen]
    require(text[:2] == b"\x60\x4e", f"missing driver branch: {path}")
    require(text[2:10] == b"OFFSCRN\x00", f"missing OFFSCRN magic: {path}")
    colours = struct.unpack(">I", text[0x40:0x44])[0]
    planes, pixel_format, organisation = struct.unpack(">HHH", text[0x44:0x4a])
    entries = {}
    for name, offset in (("init", 0x10), ("reset", 0x14), ("wk_init", 0x18),
                          ("wk_reset", 0x1c), ("opnwkinfo", 0x20),
                          ("extndinfo", 0x24), ("scrninfo", 0x28),
                          ("resinfo", 0x2c)):
        entries[name] = struct.unpack(">I", text[offset:offset + 4])[0]
    return {
        "path": path, "length": len(data), "tlen": tlen, "dlen": dlen,
        "blen": blen, "syms": syms,
        "version": text[0x0a:0x0c].hex(),
        "header_flags": data[0x18:0x1a].hex(),
        "organisation": (colours, planes, pixel_format, organisation),
        "reloc_len": len(data) - (0x1c + tlen + dlen),
        "entries": entries, "sha256": hashlib.sha256(data).hexdigest(),
    }


def structure_report(built: dict, archive: dict, release: dict, output: Path) -> None:
    require(built["sha256"] == archive["sha256"],
            "rebuilt OFF004IP.OSD does not match MIN-ATAR archive")
    require(built["organisation"] == (4, 2, 0, 1),
            f"unexpected driver organisation: {built['organisation']}")
    lines = ["rebuilt == MIN-ATAR archive (byte-identical)",
             f"rebuilt: {built['length']} bytes tlen=0x{built['tlen']:x} "
             f"dlen=0x{built['dlen']:x} blen=0x{built['blen']:x} "
             f"version={built['version']} flags={built['header_flags']} "
             f"colours/planes/format/organisation={built['organisation']}",
             f"MIN-ATAR: {archive['length']} bytes version={archive['version']} "
             f"reloc={archive['reloc_len']} bytes",
             f"Release:  {release['length']} bytes tlen=0x{release['tlen']:x} "
             f"dlen=0x{release['dlen']:x} version={release['version']}",
             "entry offsets (rebuilt): "
             + ", ".join(f"{k}=0x{v:x}" for k, v in built["entries"].items()),
             "entry offsets (release): "
             + ", ".join(f"{k}=0x{v:x}" for k, v in release["entries"].items()),
             "The Release V4.11 has a different text/data size and entry layout; "
             "the V4.10 source here cannot be a byte-identical replacement for it."]
    if release["sha256"] != built["sha256"]:
        lines.insert(1, "rebuilt != Release V4.11")
    output.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("\n".join(lines))


def expected_bitmap() -> bytes:
    """Two bars: pen 2 (raw 1) rows 2-3, pen 3 (raw 2) rows 4-5.

    Each 16-pixel group is two big-endian words: plane 0 then plane 1.
    """
    bitmap = bytearray(64 * 16)
    def word(y: int, group: int, plane: int, value: int) -> None:
        offset = y * 16 + group * 4 + plane * 2
        bitmap[offset:offset + 2] = value.to_bytes(2, "big")
    for y in (2, 3):
        word(y, 0, 0, 0x0003)          # x=14,15
        word(y, 1, 0, 0xC000)          # x=16,17
    for y in (4, 5):
        word(y, 0, 1, 0x0001)          # x=15
        word(y, 1, 1, 0xE000)          # x=16,17,18
    return bytes(bitmap)


def run_case(headless: Path, rom: Path, source: Path, driver: Path,
             label: str, output: Path, partition_offset: int) -> tuple[str, bytes]:
    case = output / label
    case.mkdir(parents=True)
    image = MagiCImage.clone(source, case / "magic.img",
                             partition_offset=partition_offset)
    image.verify_magic()
    image.copy_in(driver, "::/GEMSYS/OFF004IP.OSD")
    image.copy_in(PROBE, "::/GEMSYS/MAGIC/START/OFF004IP.PRG")
    (case / "GEMDRIVE" / "E").mkdir(parents=True)
    (case / "server").mkdir()
    options = [
        "--machine", "megaste", "--mem", "4m", "--no-disk",
        "--acsi", str(image.path), "--gemdos", str(case / "GEMDRIVE"),
        "--auto", r"C:\AUTO\MAGXBOOT.TOS",
    ]
    with NeoSTServer(headless, rom, case / "server", options=options,
                     timeout=240) as server:
        server.run_frames(8000)
        server.screenshot(case / "final.ppm")
    image.copy_out("::/GEMSYS/OFF004IP.OSD", case / "installed.OSD")
    image.copy_out("::/OFF4.TXT", case / "OFF4.TXT")
    image.copy_out("::/OFF4.BIN", case / "OFF4.BIN")
    require(hashlib.sha256((case / "installed.OSD").read_bytes()).hexdigest()
            == hashlib.sha256(driver.read_bytes()).hexdigest(),
            f"{label}: guest OFF004IP.OSD does not match the installed file")
    text = (case / "OFF4.TXT").read_text(encoding="ascii", errors="replace").strip()
    return text, (case / "OFF4.BIN").read_bytes()


def check_result(text: str) -> list[str]:
    require(text.startswith("pixels "), f"probe failed: {text}")
    samples = text.split()[1:]
    require(samples == EXPECTED_PIXELS,
            f"pixel readback {samples} != {EXPECTED_PIXELS}")
    return samples


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--neost", type=Path, default=Path.home() / ".local/bin/neost")
    parser.add_argument("--neost-headless", type=Path)
    parser.add_argument("--magic-image", type=Path,
                        default=Path(os.environ["MAGIC_IMAGE"]) if "MAGIC_IMAGE" in os.environ else None)
    parser.add_argument("--tos", type=Path,
                        default=Path(os.environ["TOS206_ROM"]) if "TOS206_ROM" in os.environ else None)
    parser.add_argument("--partition-offset", type=int, default=1024)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    if args.magic_image is None or args.tos is None:
        parser.error("pass --magic-image and --tos (or set MAGIC_IMAGE and TOS206_ROM)")

    build = subprocess.run(
        [str(ROOT / "scripts/atari-container"), "run", "make", "-f", "Makefile.apps",
         "off004ip", "off004ip-test-driver"], cwd=ROOT, text=True,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=900)
    if build.returncode:
        print(build.stdout, file=sys.stderr)
        return build.returncode
    for path in (REBUILT, PROBE):
        if not path.is_file():
            print(f"build completed without producing {path}", file=sys.stderr)
            return 1

    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-off004ip")
    except OSError as error:
        parser.error(str(error))

    try:
        built = parse_tos(REBUILT)
        archive = parse_tos(ARCHIVE)
        release = parse_tos(RELEASE)
        structure_report(built, archive, release, output / "structure.txt")

        expected = expected_bitmap()
        with tempfile.TemporaryDirectory(prefix="neost-off004ip-") as temporary:
            headless, _ = acquire_headless(args.neost_headless, args.neost, Path(temporary))
            rom = args.tos.expanduser().resolve()
            source = args.magic_image.expanduser().resolve()
            archived_text, archived_bin = run_case(
                headless, rom, source, ARCHIVE, "archive", output,
                args.partition_offset)
            rebuilt_text, rebuilt_bin = run_case(
                headless, rom, source, REBUILT, "rebuilt", output,
                args.partition_offset)

        check_result(archived_text)
        check_result(rebuilt_text)
        require(archived_text == rebuilt_text,
                f"probe text differs: archive={archived_text!r} rebuilt={rebuilt_text!r}")
        require(archived_bin == rebuilt_bin, "bitmap dump differs between archive and rebuild")
        require(rebuilt_bin == expected,
                "bitmap does not match the expected two-plane interleaved image")
        print(f"OFF004IP/NVDI probe: {rebuilt_text}")
        print(f"bitmap: {len(rebuilt_bin)} bytes, archive/rebuilt identical, matches expectation")
        print(f"Evidence: {output}")
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"OFF004IP smoke failed: {error}\nArtifacts: {output}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
