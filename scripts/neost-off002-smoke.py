#!/usr/bin/env python3
"""Build OFF002.OSD and prove MagiC/NVDI's 1-plane offscreen path.

The build must reproduce the archived V4.10 MIN-ATARI driver exactly.  The
runtime probe installs the rebuilt file as C:\\GEMSYS\\OFF002.OSD, opens a
caller-owned 64x64 one-plane MFDB through VDI v_opnbm(), fills a rectangle,
reads the pixel back and records the bitmap hash.  A successful probe therefore
depends on the rebuilt OSD being selected by NVDI; there is no other matching
one-plane OSD in the fixture.
"""
from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile

from neost_common import acquire_headless, create_output, require
from neost_image import MagiCImage
from neost_server import NeoSTServer

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
REBUILT = BUILD / "apps/osd/OFF002.OSD"
PROBE = BUILD / "apps/off002/OFF002.PRG"
MIN_ARCHIVE = ROOT / "MagiC/KERNEL/VDI/DRIVERS/OSD/MIN-ATAR/OFF002.OSD"
RELEASE = ROOT / "MagiC/Releases/MAGIC.A/GEMSYS/OFF002.OSD"


def parse_tos(path: Path) -> dict:
    data = path.read_bytes()
    require(len(data) >= 0x20, f"not a TOS executable: {path}")
    tlen, dlen, blen, syms = struct.unpack(">IIII", data[2:18])
    text = data[0x1c:0x1c + tlen]
    require(text[:2] == b"\x60\x4e", f"missing driver branch: {path}")
    require(text[2:10] == b"OFFSCRN\x00", f"missing OFFSCRN magic: {path}")
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
        "reloc_len": len(data) - (0x1c + tlen + dlen),
        "reloc_sha256": hashlib.sha256(data[0x1c + tlen + dlen:]).hexdigest(),
        "entries": entries, "sha256": hashlib.sha256(data).hexdigest(),
        "data": data,
    }


def structure_report(built: dict, archive: dict, release: dict, output: Path) -> None:
    require(built["sha256"] == archive["sha256"],
            "rebuilt OFF002.OSD does not match MIN-ATAR archive")
    lines = ["rebuilt == MIN-ATAR archive (byte-identical)",
             f"rebuilt: {built['length']} bytes tlen={built['tlen']:#x} "
             f"dlen={built['dlen']:#x} blen={built['blen']:#x} "
             f"syms={built['syms']:#x} version={built['version']} "
             f"flags={built['header_flags']}",
             f"MIN-ATAR: {archive['length']} bytes tlen={archive['tlen']:#x} "
             f"dlen={archive['dlen']:#x} version={archive['version']} "
             f"reloc={archive['reloc_len']} bytes",
             f"Release:  {release['length']} bytes tlen={release['tlen']:#x} "
             f"dlen={release['dlen']:#x} version={release['version']} "
             f"reloc={release['reloc_len']} bytes",
             "entry offsets (rebuilt): " + ", ".join(
                 f"{name}={value:#x}" for name, value in built["entries"].items()),
             "release entry offsets: " + ", ".join(
                 f"{name}={value:#x}" for name, value in release["entries"].items()),
             "The Release V4.11 has a different text/data size and entry layout; "
             "the V4.10 source here cannot be a byte-identical replacement for it."]
    output.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("\n".join(lines))


def run_probe(headless: Path, rom: Path, source: Path, output: Path) -> str:
    image = MagiCImage.clone(source, output / "magic.img")
    image.verify_magic()
    image.copy_in(REBUILT, "::/GEMSYS/OFF002.OSD")
    image.copy_in(PROBE, "::/GEMSYS/MAGIC/START/OFF002.PRG")
    (output / "GEMDRIVE" / "E").mkdir(parents=True)
    (output / "server").mkdir()
    options = [
        "--machine", "megaste", "--mem", "4m", "--no-disk",
        "--acsi", str(image.path), "--gemdos", str(output / "GEMDRIVE"),
        "--auto", r"C:\AUTO\MAGXBOOT.TOS",
    ]
    with NeoSTServer(headless, rom, output / "server", options=options,
                     timeout=240) as server:
        server.run_frames(8000)
        server.screenshot(output / "final.ppm")
    image.copy_out("::/GEMSYS/OFF002.OSD", output / "installed.OSD")
    image.copy_out("::/OFF002.TXT", output / "OFF002.TXT")
    return (output / "OFF002.TXT").read_text(encoding="ascii", errors="replace").strip()


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
         "off002", "off002-test-driver"], cwd=ROOT, text=True,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=900)
    if build.returncode:
        print(build.stdout, file=sys.stderr)
        return build.returncode
    for path in (REBUILT, PROBE):
        if not path.is_file():
            print(f"build completed without producing {path}", file=sys.stderr)
            return 1

    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-off002")
    except OSError as error:
        parser.error(str(error))

    try:
        built = parse_tos(REBUILT)
        archive = parse_tos(MIN_ARCHIVE)
        release = parse_tos(RELEASE)
        structure_report(built, archive, release, output / "structure.txt")

        with tempfile.TemporaryDirectory(prefix="neost-off002-") as temporary:
            headless, _ = acquire_headless(args.neost_headless, args.neost, Path(temporary))
            result = run_probe(headless, args.tos.expanduser().resolve(),
                               args.magic_image.expanduser().resolve(), output)
        installed = output / "installed.OSD"
        require(hashlib.sha256(installed.read_bytes()).hexdigest() == built["sha256"],
                "guest OFF002.OSD does not match the rebuilt file")
        require(result.startswith("ok handle="), f"probe failed: {result}")
        fields = dict(part.split("=", 1) for part in result.split()[1:])
        require(fields.get("pixel") == "1" and fields.get("index") == "1",
                f"1-plane rectangle was not drawn: {result}")
        require(fields.get("hash") == "a7d909e5",
                f"bitmap hash changed: {result}")
        print(f"OFF002/NVDI probe: {result}")
        print(f"Evidence: {output}")
    except (OSError, RuntimeError, subprocess.SubprocessError) as error:
        print(f"OFF002 smoke failed: {error}\nArtifacts: {output}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
