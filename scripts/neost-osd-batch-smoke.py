#!/usr/bin/env python3
"""Build and verify one remaining OSD driver against its reference and NVDI.

Each driver requests its own bitmap organisation explicitly, installs archive
and rebuild as C:\\GEMSYS\\<driver>.OSD in separate MagiC clones, draws known
pens, and compares pixel readbacks and the full bitmap against an independently
constructed layout.  A clone with the driver deleted must report the open as
rejected.  The exact binary reference is used only where the source version and
layout match it; otherwise the structure is recorded and runtime behavior is
checked.
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
OSD = ROOT / "MagiC/KERNEL/VDI/DRIVERS/OSD"
RELEASE = ROOT / "MagiC/Releases/MAGIC.A/GEMSYS"


def s16(value: int) -> int:
    return value - 0x10000 if value & 0x8000 else value


def s32(value: int) -> int:
    return value - 0x100000000 if value & 0x80000000 else value


def swap32(value: int) -> int:
    return ((value << 16) & 0xFFFFFFFF) | (value >> 16)


def bitmap_016pk() -> bytes:
    bitmap = bytearray(64 * 32)
    for i, raw in enumerate((1, 2, 4, 8, 15)):
        for y in (2 + 2 * i, 3 + 2 * i):
            bitmap[y * 32 + 7] = raw               # x=15, low nibble
            bitmap[y * 32 + 8] = raw << 4          # x=16, high nibble
    return bytes(bitmap)


def bitmap_256pk() -> bytes:
    bitmap = bytearray(64 * 64)
    raws = (1, 2, 4, 8, 16, 32, 64, 128, 255, 6)
    for i, raw in enumerate(raws):
        for y in (2 + 2 * i, 3 + 2 * i):
            bitmap[y * 64 + 14:y * 64 + 18] = bytes([raw] * 4)
    return bytes(bitmap)


def bitmap_32k(values) -> bytes:
    bitmap = bytearray(64 * 128)
    for i, value in enumerate(values):
        for y in (2 + 2 * i, 3 + 2 * i):
            for x in range(14, 18):
                offset = y * 128 + x * 2
                bitmap[offset:offset + 2] = value.to_bytes(2, "big")
    return bytes(bitmap)


def bitmap_16mov() -> bytes:
    bitmap = bytearray(64 * 256)
    pixels = (b"\x00\x00\xff\x00", b"\x00\xff\x00\x00", b"\xff\x00\x00\x00",
              b"\x00\x80\xff\x00", b"\xff\x00\x80\x00")
    for i, pixel in enumerate(pixels):
        for y in (2 + 2 * i, 3 + 2 * i):
            for x in range(14, 18):
                offset = y * 256 + x * 4
                bitmap[offset:offset + 4] = pixel
    return bytes(bitmap)


def pixels(values) -> list[str]:
    out = [f"{s16(v)}/-1" for v in values]
    return [item for item in out for _ in (0, 1)] + ["0/-1"]


V32FL = (0xF800, 0x07C0, 0x001F, 0xFC00, 0x801F)
V32PK_BASE = (0x7C00, 0x03E0, 0x001F, 0x7E00, 0x401F)
V32PK = tuple(((v << 8) | (v >> 8)) & 0xFFFF for v in V32PK_BASE)
V16MOV_RAW = (0x0000FF00, 0x00FF0000, 0xFF000000, 0x0080FF00, 0xFF008000)


def long_pixels(values) -> list[str]:
    out = [f"{s32(swap32(v))}/-1" for v in values]
    return [item for item in out for _ in (0, 1)] + ["0/-1"]

PROFILES = {
    "off016pk": {
        "osd": "OFF016PK", "version": "0410", "colors": 16, "planes": 4,
        "format": 2, "org": 1,
        "exact": OSD / "OFF016PK.OSD",
        "release": RELEASE / "OFF016PK.OSD",
        "text": "OFF16PK", "expected_pixels": [
            "1/2", "1/2", "2/3", "2/3", "4/4", "4/4", "8/9", "8/9",
            "15/1", "15/1", "0/0"],
        "bitmap": bitmap_016pk,
    },
    "off256pk": {
        "osd": "OFF256PK", "version": "0411", "colors": 256, "planes": 8,
        "format": 2, "org": 1,
        "exact": None,
        "release": RELEASE / "OFF256PK.OSD",
        "text": "OFF256PK", "expected_pixels": [
            "1/2", "1/2", "2/3", "2/3", "4/4", "4/4", "8/9", "8/9",
            "16/16", "16/16", "32/32", "32/32", "64/64", "64/64",
            "128/128", "128/128", "255/1", "255/1", "6/5", "6/5", "0/0"],
        "bitmap": bitmap_256pk,
    },
    "off32kfl": {
        "osd": "OFF32KFL", "version": "0411", "colors": 32768, "planes": 16,
        "format": 2, "org": 2,
        "exact": RELEASE / "OFF32KFL.OSD",
        "archive": OSD / "MIN-ATAR/OFF32KFL.OSD",
        "release": RELEASE / "OFF32KFL.OSD",
        "text": "OFF32KF", "expected_pixels": pixels(V32FL),
        "bitmap": lambda: bitmap_32k(V32FL),
    },
    "off32kpk": {
        "osd": "OFF32KPK", "version": "0411", "colors": 32768, "planes": 16,
        "format": 2, "org": 129,
        "exact": None,
        "release": RELEASE / "OFF32KPK.OSD",
        "text": "OFF32KP", "expected_pixels": pixels(V32PK),
        "bitmap": lambda: bitmap_32k(V32PK),
    },
    "off16mov": {
        "osd": "OFF16MOV", "version": "0413", "colors": 16777216,
        "planes": 32, "format": 2, "org": 129,
        "exact": None,
        "release": RELEASE / "OFF16MOV.OSD",
        "text": "OFF16MV", "expected_pixels": long_pixels(V16MOV_RAW),
        "bitmap": bitmap_16mov,
    },
}


def parse_tos(path: Path) -> dict:
    data = path.read_bytes()
    require(len(data) >= 0x20, f"not a TOS executable: {path}")
    tlen, dlen, blen, syms = struct.unpack(">IIII", data[2:18])
    text = data[0x1c:0x1c + tlen]
    require(text[:2] == b"\x60\x4e", f"missing driver branch: {path}")
    require(text[2:10] == b"OFFSCRN\x00", f"missing OFFSCRN magic: {path}")
    colours = struct.unpack(">I", text[0x40:0x44])[0]
    planes, pixel_format, organisation = struct.unpack(">HHH", text[0x44:0x4a])
    entries = [struct.unpack(">I", text[o:o + 4])[0]
               for o in range(0x10, 0x30, 4)]
    return {
        "path": path, "length": len(data), "tlen": tlen, "dlen": dlen,
        "blen": blen, "syms": syms, "version": text[0x0a:0x0c].hex(),
        "organisation": (colours, planes, pixel_format, organisation),
        "entries": entries, "sha256": hashlib.sha256(data).hexdigest(),
    }


def structure_report(profile: dict, built: dict, output: Path) -> None:
    require(built["organisation"] ==
            (profile["colors"], profile["planes"], profile["format"], profile["org"]),
            f"unexpected organisation: {built['organisation']}")
    require(built["version"] == profile["version"],
            f"unexpected version {built['version']}")
    lines = [f"rebuilt: {built['length']} bytes tlen=0x{built['tlen']:x} "
             f"dlen=0x{built['dlen']:x} version={built['version']} "
             f"organisation={built['organisation']}"]
    archive = profile.get("archive", OSD / (profile["osd"] + ".OSD"))
    for label, path in (("source archive", archive), ("release", profile["release"])):
        if not path.is_file():
            lines.append(f"{label}: missing ({path})")
            continue
        ref = parse_tos(path)
        lines.append(f"{label}: {ref['length']} bytes version={ref['version']} "
                     f"organisation={ref['organisation']} equal={ref['sha256'] == built['sha256']}")
    if profile["exact"] is not None:
        exact = parse_tos(profile["exact"])
        require(built["sha256"] == exact["sha256"],
                f"rebuilt {profile['osd']} does not match {profile['exact']}")
        lines.insert(0, f"rebuilt == {profile['exact'].parent.name} (byte-identical)")
    else:
        lines.insert(0, "no byte-identical reference for this source version")
    output.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("\n".join(lines))


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


def run_case(profile: dict, headless: Path, rom: Path, source: Path,
             driver: Path | None, label: str, output: Path,
             partition_offset: int) -> tuple[str, bytes | None]:
    osd = profile["osd"]
    text_name = profile["text"] + ".TXT"
    bin_name = profile["text"] + ".BIN"
    case = output / label
    case.mkdir(parents=True)
    image = MagiCImage.clone(source, case / "magic.img",
                             partition_offset=partition_offset)
    image.verify_magic()
    for guest in (f"::/{text_name}", f"::/{bin_name}"):
        remove_guest(image, guest)
    if driver is None:
        remove_guest(image, f"::/GEMSYS/{osd}.OSD")
    else:
        image.copy_in(driver, f"::/GEMSYS/{osd}.OSD")
    image.copy_in(BUILD / "apps/osd-probes" / (osd + ".PRG"),
                  f"::/GEMSYS/MAGIC/START/{osd}.PRG")
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
        image.copy_out(f"::/GEMSYS/{osd}.OSD", case / "installed.OSD")
        require(hashlib.sha256((case / "installed.OSD").read_bytes()).hexdigest()
                == hashlib.sha256(driver.read_bytes()).hexdigest(),
                f"{label}: guest {osd}.OSD does not match the installed file")
    require(guest_present(image, f"::/{text_name}"),
            f"{label}: probe did not write {text_name}")
    image.copy_out(f"::/{text_name}", case / text_name)
    text = (case / text_name).read_text(encoding="ascii", errors="replace").strip()
    if driver is None:
        require(not guest_present(image, f"::/{bin_name}"),
                f"{label}: missing driver produced {bin_name}")
        bitmap = None
    else:
        require(guest_present(image, f"::/{bin_name}"),
                f"{label}: probe did not write {bin_name}")
        image.copy_out(f"::/{bin_name}", case / bin_name)
        bitmap = (case / bin_name).read_bytes()
    return text, bitmap


def check_result(profile: dict, text: str) -> None:
    require(text.startswith("pixels "), f"probe failed: {text}")
    samples = text.split()[1:]
    require(samples == profile["expected_pixels"],
            f"pixel readback {samples} != {profile['expected_pixels']}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--driver", required=True, choices=sorted(PROFILES))
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

    profile = PROFILES[args.driver]
    osd = profile["osd"]
    rebuilt = BUILD / "apps/osd" / (osd + ".OSD")

    build = subprocess.run(
        [str(ROOT / "scripts/atari-container"), "run", "make", "-f", "Makefile.apps",
         args.driver, args.driver + "-test-driver"], cwd=ROOT, text=True,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=900)
    if build.returncode:
        print(build.stdout, file=sys.stderr)
        return build.returncode
    for path in (rebuilt, BUILD / "apps/osd-probes" / (osd + ".PRG")):
        if not path.is_file():
            print(f"build completed without producing {path}", file=sys.stderr)
            return 1

    try:
        output = create_output(args.output, parent=BUILD, prefix=f"neost-{args.driver}")
    except OSError as error:
        parser.error(str(error))

    try:
        built = parse_tos(rebuilt)
        structure_report(profile, built, output / "structure.txt")

        expected = profile["bitmap"]()
        with tempfile.TemporaryDirectory(prefix=f"neost-{args.driver}-") as temporary:
            headless, _ = acquire_headless(args.neost_headless, args.neost, Path(temporary))
            rom = args.tos.expanduser().resolve()
            source = args.magic_image.expanduser().resolve()
            archive = profile.get("archive", OSD / (osd + ".OSD"))
            archived_text, archived_bin = run_case(
                profile, headless, rom, source, archive, "archive", output,
                args.partition_offset)
            rebuilt_text, rebuilt_bin = run_case(
                profile, headless, rom, source, rebuilt, "rebuilt", output,
                args.partition_offset)
            missing_text, missing_bin = run_case(
                profile, headless, rom, source, None, "missing", output,
                args.partition_offset)

        check_result(profile, archived_text)
        check_result(profile, rebuilt_text)
        require(archived_text == rebuilt_text,
                f"probe text differs: archive={archived_text!r} rebuilt={rebuilt_text!r}")
        require(archived_bin is not None and archived_bin == rebuilt_bin,
                "bitmap dump differs between archive and rebuild")
        require(rebuilt_bin == expected,
                "bitmap does not match the independent expected layout")
        require(missing_text == "bitmap open rejected",
                f"missing driver was not rejected: {missing_text!r}")
        require(missing_bin is None, "missing driver still produced a bitmap dump")
        print(f"{osd}/NVDI probe: {rebuilt_text}")
        print(f"bitmap: {len(rebuilt_bin)} bytes, archive/rebuilt identical, matches expectation")
        print("missing-driver control: open rejected, no bitmap written")
        print(f"Evidence: {output}")
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"{osd} smoke failed: {error}\nArtifacts: {output}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
