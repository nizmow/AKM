#!/usr/bin/env python3
"""Build OFF016IP.OSD and prove MagiC/NVDI's 4-plane offscreen path.

The rebuild must reproduce the archived V4.10 MIN-ATARI driver exactly.  The
probe requests a 16-colour, 4-plane, non-packed interleaved bitmap explicitly
and installs archive/rebuild as C:\\GEMSYS\\OFF016IP.OSD in separate MagiC
clones.  It draws five bars with VDI pens 2, 3, 4, 9 and 1 (raw colours 1, 2,
4, 8, 15), samples both sides of the x=15/16 word boundary, dumps the
2048-byte bitmap and checks an untouched pixel.  A third clone deletes
OFF016IP.OSD; no other installed driver matches this request, so v_opnbm() must
be rejected rather than fall back.
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

from neost_common import acquire_headless, create_output, require, run
from neost_image import MagiCImage
from neost_server import NeoSTServer

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
REBUILT = BUILD / "apps/osd/OFF016IP.OSD"
PROBE = BUILD / "apps/off016ip/OFF016IP.PRG"
ARCHIVE = ROOT / "MagiC/KERNEL/VDI/DRIVERS/OSD/MIN-ATAR/OFF016IP.OSD"
RELEASE = ROOT / "MagiC/Releases/MAGIC.A/GEMSYS/OFF016IP.OSD"

# pel is the raw bitmap colour; index is the VDI pen (color_to_vdi).
EXPECTED_PIXELS = ["1/2", "1/2", "2/3", "2/3", "4/4", "4/4",
                   "8/9", "8/9", "15/1", "15/1", "0/0"]
PENS = (2, 3, 4, 9, 1)


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
            "rebuilt OFF016IP.OSD does not match MIN-ATAR archive")
    require(built["organisation"] == (16, 4, 0, 1),
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
             f"The Release driver ({release['version']}) has a different "
             "text/data size and entry layout; the V4.10 source here cannot be "
             "a byte-identical replacement for it."]
    if release["sha256"] != built["sha256"]:
        lines.insert(1, f"rebuilt != Release {release['version']}")
    output.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("\n".join(lines))


def expected_bitmap() -> bytes:
    """Bars at rows 2..11: one plane per pen, then all four for pen 1.

    Each 16-pixel group holds four big-endian plane words, plane 0 first.
    """
    bitmap = bytearray(64 * 32)
    def word(y: int, group: int, plane: int, value: int) -> None:
        offset = y * 32 + group * 8 + plane * 2
        bitmap[offset:offset + 2] = value.to_bytes(2, "big")
    planes = {0: (0,), 1: (1,), 2: (2,), 3: (3,), 4: (0, 1, 2, 3)}
    for i, plane_set in planes.items():
        for y in (2 + 2 * i, 3 + 2 * i):
            for plane in plane_set:
                word(y, 0, plane, 0x0003)      # x=14,15
                word(y, 1, plane, 0xC000)      # x=16,17
    return bytes(bitmap)


def run_case(headless: Path, rom: Path, source: Path, driver: Path | None,
             label: str, output: Path, partition_offset: int) -> tuple[str, bytes | None]:
    case = output / label
    case.mkdir(parents=True)
    image = MagiCImage.clone(source, case / "magic.img",
                             partition_offset=partition_offset)
    image.verify_magic()
    if driver is None:
        result = run(["mdel", "-i", image.image_spec, "::/GEMSYS/OFF016IP.OSD"])
        if result.returncode:
            check = run(["mdir", "-i", image.image_spec, "::/GEMSYS/OFF016IP.OSD"])
            if check.returncode == 0:
                raise RuntimeError(f"could not remove OFF016IP.OSD: {result.stdout}")
    else:
        image.copy_in(driver, "::/GEMSYS/OFF016IP.OSD")
    image.copy_in(PROBE, "::/GEMSYS/MAGIC/START/OFF016IP.PRG")
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
    if driver is not None:
        image.copy_out("::/GEMSYS/OFF016IP.OSD", case / "installed.OSD")
        require(hashlib.sha256((case / "installed.OSD").read_bytes()).hexdigest()
                == hashlib.sha256(driver.read_bytes()).hexdigest(),
                f"{label}: guest OFF016IP.OSD does not match the installed file")
    image.copy_out("::/OFF16.TXT", case / "OFF16.TXT")
    text = (case / "OFF16.TXT").read_text(encoding="ascii", errors="replace").strip()
    try:
        image.copy_out("::/OFF16.BIN", case / "OFF16.BIN")
        bitmap = (case / "OFF16.BIN").read_bytes()
    except RuntimeError:
        bitmap = None
    return text, bitmap


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
         "off016ip", "off016ip-test-driver"], cwd=ROOT, text=True,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=900)
    if build.returncode:
        print(build.stdout, file=sys.stderr)
        return build.returncode
    for path in (REBUILT, PROBE):
        if not path.is_file():
            print(f"build completed without producing {path}", file=sys.stderr)
            return 1

    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-off016ip")
    except OSError as error:
        parser.error(str(error))

    try:
        built = parse_tos(REBUILT)
        archive = parse_tos(ARCHIVE)
        release = parse_tos(RELEASE)
        structure_report(built, archive, release, output / "structure.txt")

        expected = expected_bitmap()
        with tempfile.TemporaryDirectory(prefix="neost-off016ip-") as temporary:
            headless, _ = acquire_headless(args.neost_headless, args.neost, Path(temporary))
            rom = args.tos.expanduser().resolve()
            source = args.magic_image.expanduser().resolve()
            archived_text, archived_bin = run_case(
                headless, rom, source, ARCHIVE, "archive", output,
                args.partition_offset)
            rebuilt_text, rebuilt_bin = run_case(
                headless, rom, source, REBUILT, "rebuilt", output,
                args.partition_offset)
            missing_text, missing_bin = run_case(
                headless, rom, source, None, "missing", output,
                args.partition_offset)

        check_result(archived_text)
        check_result(rebuilt_text)
        require(archived_text == rebuilt_text,
                f"probe text differs: archive={archived_text!r} rebuilt={rebuilt_text!r}")
        require(archived_bin is not None and archived_bin == rebuilt_bin,
                "bitmap dump differs between archive and rebuild")
        require(rebuilt_bin == expected,
                "bitmap does not match the expected four-plane interleaved image")
        require(missing_text == "bitmap open rejected",
                f"missing driver was not rejected: {missing_text!r}")
        require(missing_bin is None, "missing driver still produced a bitmap dump")
        print(f"OFF016IP/NVDI probe: {rebuilt_text}")
        print(f"bitmap: {len(rebuilt_bin)} bytes, archive/rebuilt identical, matches expectation")
        print("missing-driver control: v_opnbm rejected, no bitmap written")
        print(f"Evidence: {output}")
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"OFF016IP smoke failed: {error}\nArtifacts: {output}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
