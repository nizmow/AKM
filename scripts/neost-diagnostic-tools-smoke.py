#!/usr/bin/env python3
"""Compare the archived and rebuilt MagiC diagnostic tools on cloned images."""
from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

from neost_common import acquire_headless, create_output, require, run
from neost_image import MagiCImage

ROOT = Path(__file__).resolve().parent.parent
BUILD = ROOT / "build/atari"
TOOLS = {
    "MAGXKERN": ("MAGXKERN/BIN/MAGXKERN.TTP", "DIAGTOOL.TTP"),
    "MEMEXAMN": ("MEMEXAMN/BIN/MEMEXAMN.TTP", "DIAGTOOL.TTP"),
    "CRASHDMP": ("CRASHDMP/BIN/CRASHDMP.TOS", "DIAGTOOL.TOS"),
}
def boot_image(label: str, source: Path, output: Path, headless: Path,
               rom: Path, partition_offset: int, *, driver: Path | None,
               guest: str | None) -> tuple[Path, bytes | None]:
    case = output / label
    case.mkdir(parents=True)
    image = MagiCImage.clone(source, case / "magic.img",
                             partition_offset=partition_offset)
    image.verify_magic()
    if driver is not None:
        image.copy_in(driver, f"::/{guest}")
    image.copy_in(BUILD / "apps/diagnostic/DIAG.PRG",
                  "::/GEMSYS/MAGIC/START/DIAG.PRG")
    config = case / "DIAG.TXT"
    config.write_bytes(f"C:\\{guest}\r\n".encode())
    image.copy_in(config, "::/DIAG.TXT")
    (case / "GEMDRIVE" / "E").mkdir(parents=True)
    command = [
        str(headless), "--machine", "megaste", "--mem", "4m", "--mono",
        "--no-disk", "--acsi", str(image.path), "--gemdos", str(case / "GEMDRIVE"),
        "--auto", r"C:\AUTO\MAGXBOOT.TOS", "--frames", "16000",
        "--screenshot", str(case / "final.ppm"), str(rom),
    ]
    result = run(command, timeout=120)
    require(result.returncode == 0, f"{label}: NeoST failed")
    status = b""
    for name in ("DIAG.STA", "DIAG.OUT"):
        try:
            image.copy_out(f"::/{name}", case / name)
        except RuntimeError:
            pass
    if (case / "DIAG.STA").is_file():
        status = (case / "DIAG.STA").read_bytes()
    return case / "final.ppm", status


def read_output(case: Path) -> bytes:
    path = case / "DIAG.OUT"
    return path.read_bytes() if path.is_file() else b""


def normalize(data: bytes) -> bytes:
    return re.sub(rb"[0-9a-f]{3,8}", b"#", data)


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
         "showfree", "magxkern", "memexamn", "crashdmp", "diagnostic-test-driver"],
        cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
        timeout=600)
    if build.returncode:
        print(build.stdout, file=sys.stderr)
        return build.returncode
    try:
        output = create_output(args.output, parent=BUILD, prefix="neost-diagnostic")
    except OSError as error:
        parser.error(str(error))

    with tempfile.TemporaryDirectory(prefix="neost-diagnostic-") as temporary:
        headless, _ = acquire_headless(args.neost_headless, args.neost, Path(temporary))
        rom = args.tos.expanduser().resolve()
        source = args.magic_image.expanduser().resolve()
        for case in ("MAGXKERN", "MEMEXAMN", "CRASHDMP"):
            archive_src, guest = TOOLS[case]
            archive = ROOT / "MagiC/TOOLS" / archive_src
            rebuilt = BUILD / "apps" / case.lower() / (case + Path(archive_src).suffix)
            require(rebuilt.is_file(), f"missing rebuilt {rebuilt}")
            a_shot, a_status = boot_image("archive-" + case.lower(), source, output,
                                          headless, rom, args.partition_offset,
                                          driver=archive, guest=guest)
            r_shot, r_status = boot_image("rebuilt-" + case.lower(), source, output,
                                          headless, rom, args.partition_offset,
                                          driver=rebuilt, guest=guest)
            a_out, r_out = read_output(output / ("archive-" + case.lower())), \
                read_output(output / ("rebuilt-" + case.lower()))
            require(a_status == r_status == b"0\n", f"{case}: child status differs")
            if case == "MAGXKERN":
                require(a_out == r_out, "MAGXKERN output differs")
            elif case == "CRASHDMP":
                require(a_out == r_out == b"", "CRASHDMP wrote unexpected output")
            else:
                for data in (a_out, r_out):
                    require(b"Table 0" in data and b"boot" in data and
                            b"AES" in data and b"free" in data and
                            b"DIAGTOOL.TTP" in data, "MEMEXAMN structure differs")
                require(normalize(a_out).splitlines()[:30] ==
                        normalize(r_out).splitlines()[:30],
                        "MEMEXAMN header/early table differs")
            print(f"{case}: archive/rebuilt status 0, output checked")

        # SHOWFREE loops until a key; compare the booted console and screen.
        for label, binary in (
            ("archive-showfree", ROOT / "MagiC/TOOLS/SHOWFREE/SHOWFREE.TOS"),
            ("rebuilt-showfree", BUILD / "apps/showfree/SHOWFREE.TOS"),
        ):
            case = output / label
            case.mkdir(parents=True)
            image = MagiCImage.clone(source, case / "magic.img",
                                     partition_offset=args.partition_offset)
            image.verify_magic()
            image.copy_in(binary, "::/GEMSYS/MAGIC/START/SHOWFREE.PRG")
            (case / "GEMDRIVE" / "E").mkdir(parents=True)
            command = [
                str(headless), "--machine", "megaste", "--mem", "4m", "--mono",
                "--no-disk", "--acsi", str(image.path),
                "--gemdos", str(case / "GEMDRIVE"), "--auto", r"C:\AUTO\MAGXBOOT.TOS",
                "--console-log", str(case / "console.log"), "--frames", "9000",
                "--screenshot", str(case / "final.ppm"), str(rom),
            ]
            result = run(command, timeout=120)
            require(result.returncode == 0, f"{label}: NeoST failed")
            text = (case / "console.log").read_text(encoding="latin-1")
            # NeoST's console log duplicates rebuilt MiNTLib output; collapse it.
            collapsed = re.sub(r"(.)\1", r"\1", text)
            samples = (text, collapsed)
            require(any("MagiC Internal Memory Monitor" in sample and
                        "blocks used" in sample and "Error" not in sample
                        for sample in samples),
                    f"{label}: unexpected monitor output")
        require((output / "archive-showfree/final.ppm").read_bytes() ==
                (output / "rebuilt-showfree/final.ppm").read_bytes(),
                "SHOWFREE screens differ")
        print("SHOWFREE: archive/rebuilt screen identical")
    print(f"Evidence: {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
