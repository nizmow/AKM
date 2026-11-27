#!/usr/bin/env python3
"""Build OFF256IP.OSD and prove MagiC/NVDI's 8-plane offscreen path.

The rebuild must reproduce the archived V4.10 MIN-ATAR driver exactly.  The
probe requests a 256-colour, 8-plane, non-packed interleaved bitmap explicitly
and installs archive/rebuild as C:\\GEMSYS\\OFF256IP.OSD in separate MagiC
clones.  It draws ten bars with VDI pens 2, 3, 4, 9, 16, 32, 64, 128, 1 and 5
(raw colours 1, 2, 4, 8, 16, 32, 64, 128, 255 and 6), samples both sides of the
x=15/16 word boundary, dumps the 4096-byte bitmap and checks an untouched
pixel.  A third clone deletes OFF256IP.OSD; the open must be reported as
rejected and no bitmap dump may be retrievable.
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

from osd_release_metadata import release_metadata
from neost_common import acquire_headless, create_output, require, run
from neost_image import MagiCImage
from neost_server import NeoSTServer

ROOT = Path(__file__).resolve().parents[3]
BUILD = ROOT / "build/atari"
REBUILT = BUILD / "apps/osd/OFF256IP.OSD"
PROBE = BUILD / "apps/off256ip/OFF256IP.PRG"
ARCHIVE = ROOT / "core/osd/tests/archive/MIN-ATAR/OFF256IP.OSD"
RELEASE = release_metadata("OFF256IP")

# pel is the raw bitmap colour; index is the VDI pen (color_to_vdi).
EXPECTED_PIXELS = ["1/2", "1/2", "2/3", "2/3", "4/4", "4/4",
                   "8/9", "8/9", "16/16", "16/16", "32/32", "32/32",
                   "64/64", "64/64", "128/128", "128/128", "255/1", "255/1",
                   "6/5", "6/5", "0/0"]
RAW_COLOURS = (1, 2, 4, 8, 16, 32, 64, 128, 255, 6)


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
            "rebuilt OFF256IP.OSD does not match MIN-ATAR archive")
    require(built["organisation"] == (256, 8, 0, 1),
            f"unexpected driver organisation: {built['organisation']}")
    lines = ["rebuilt == MIN-ATAR archive (byte-identical)",
             f"rebuilt: {built['length']} bytes tlen=0x{built['tlen']:x} "
             f"dlen=0x{built['dlen']:x} blen=0x{built['blen']:x} "
             f"version={built['version']} flags={built['header_flags']} "
             f"colours/planes/format/organisation={built['organisation']}",
             f"MIN-ATAR: {archive['length']} bytes version={archive['version']} "
             f"reloc={archive['reloc_len']} bytes",
             f"Release reference: {release['length']} bytes tlen=0x{release['tlen']:x} "
             f"dlen=0x{release['dlen']:x} version={release['version']} "
             f"reloc={release['reloc_len']} bytes; {release['path']}",
             "entry offsets (rebuilt): "
             + ", ".join(f"{k}=0x{v:x}" for k, v in built["entries"].items()),
             "entry offsets (release): "
             + ", ".join(f"{k}=0x{v:x}" for k, v in release["entries"].items())]
    if release["sha256"] == built["sha256"]:
        lines.append("The Release driver is the same V4.10 binary as MIN-ATAR.")
    else:
        lines.append(f"The Release driver ({release['version']}) differs from the "
                     "V4.10 source here and is recorded structurally only.")
    output.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("\n".join(lines))


def expected_bitmap() -> bytes:
    """Bars at rows 2..21: one raw colour per bar.

    Each 16-pixel group holds eight big-endian plane words, plane 0 first.
    """
    bitmap = bytearray(64 * 64)
    def word(y: int, group: int, plane: int, value: int) -> None:
        offset = y * 64 + group * 16 + plane * 2
        bitmap[offset:offset + 2] = value.to_bytes(2, "big")
    for i, colour in enumerate(RAW_COLOURS):
        for y in (2 + 2 * i, 3 + 2 * i):
            for plane in range(8):
                if colour & (1 << plane):
                    word(y, 0, plane, 0x0003)      # x=14,15
                    word(y, 1, plane, 0xC000)      # x=16,17
    return bytes(bitmap)


def guest_present(image: MagiCImage, guest: str) -> bool:
    result = run(["mdir", "-i", image.image_spec, guest])
    if result.returncode == 0:
        return True
    if "not found" in result.stdout.lower():
        return False
    raise RuntimeError(f"could not list {guest}: {result.stdout.strip()}")


def remove_guest(image: MagiCImage, guest: str) -> None:
    run(["mdel", "-i", image.image_spec, guest])
    require(not guest_present(image, guest), f"{guest} is still present")


def run_case(headless: Path, rom: Path, source: Path, driver: Path | None,
             label: str, output: Path, partition_offset: int) -> tuple[str, bytes | None]:
    case = output / label
    case.mkdir(parents=True)
    image = MagiCImage.clone(source, case / "magic.img",
                             partition_offset=partition_offset)
    image.verify_magic()
    for guest in ("::/OFF256.TXT", "::/OFF256.BIN"):
        remove_guest(image, guest)
    if driver is None:
        remove_guest(image, "::/GEMSYS/OFF256IP.OSD")
    else:
        image.copy_in(driver, "::/GEMSYS/OFF256IP.OSD")
    image.copy_in(PROBE, "::/GEMSYS/MAGIC/START/OFF256IP.PRG")
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
        image.copy_out("::/GEMSYS/OFF256IP.OSD", case / "installed.OSD")
        require(hashlib.sha256((case / "installed.OSD").read_bytes()).hexdigest()
                == hashlib.sha256(driver.read_bytes()).hexdigest(),
                f"{label}: guest OFF256IP.OSD does not match the installed file")
    require(guest_present(image, "::/OFF256.TXT"),
            f"{label}: probe did not write OFF256.TXT")
    image.copy_out("::/OFF256.TXT", case / "OFF256.TXT")
    text = (case / "OFF256.TXT").read_text(encoding="ascii", errors="replace").strip()
    if driver is None:
        require(not guest_present(image, "::/OFF256.BIN"),
                f"{label}: missing driver produced OFF256.BIN")
        bitmap = None
    else:
        require(guest_present(image, "::/OFF256.BIN"),
                f"{label}: probe did not write OFF256.BIN")
        image.copy_out("::/OFF256.BIN", case / "OFF256.BIN")
        bitmap = (case / "OFF256.BIN").read_bytes()
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
         "off256ip", "off256ip-test-driver"], cwd=ROOT, text=True,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=900)
    if build.returncode:
        print(build.stdout, file=sys.stderr)
        return build.returncode
    for path in (REBUILT, PROBE):
        if not path.is_file():
            print(f"build completed without producing {path}", file=sys.stderr)
            return 1

    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-off256ip")
    except OSError as error:
        parser.error(str(error))

    try:
        built = parse_tos(REBUILT)
        archive = parse_tos(ARCHIVE)
        release = RELEASE
        structure_report(built, archive, release, output / "structure.txt")

        expected = expected_bitmap()
        with tempfile.TemporaryDirectory(prefix="neost-off256ip-") as temporary:
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
                "bitmap does not match the expected eight-plane interleaved image")
        require(missing_text == "bitmap open rejected",
                f"missing driver was not rejected: {missing_text!r}")
        require(missing_bin is None, "missing driver still produced a bitmap dump")
        print(f"OFF256IP/NVDI probe: {rebuilt_text}")
        print(f"bitmap: {len(rebuilt_bin)} bytes, archive/rebuilt identical, matches expectation")
        print("missing-driver control: v_opnbm rejected, no bitmap written")
        print(f"Evidence: {output}")
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"OFF256IP smoke failed: {error}\nArtifacts: {output}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
