#!/usr/bin/env python3
"""Public locale build checks: kernel regions and UK fallback emitter agreement.

Only repository inputs are used; no MagiC image or proprietary ROM is needed.
"""
from __future__ import annotations

import argparse
import re
import struct
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GEN = ROOT / "base/keyb-sys/tools/gen_keymaps.py"
REGIONS = {"locale-de": 1, "locale-en": 3}


def bytes_from_asm(text: str) -> bytes:
    values = [int(value, 16) for value in re.findall(r"\$([0-9a-fA-F]{2})", text)]
    return bytes(values)


def bytes_from_c(text: str) -> bytes:
    values = [int(value, 16) for value in re.findall(r"0x([0-9a-fA-F]{2})", text)]
    return bytes(values)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build_dir", type=Path)
    args = parser.parse_args()
    build = args.build_dir.resolve()
    for name, country in REGIONS.items():
        path = build / name / "MAGIC.RAM"
        if not path.is_file():
            print(f"missing kernel: {path}", file=sys.stderr)
            return 1
        data = path.read_bytes()
        actual = struct.unpack_from(">H", data, 56)[0] >> 1
        if actual != country:
            print(f"{name}: kernel region {actual}, expected {country}", file=sys.stderr)
            return 1
    outputs = []
    for kind, suffix in (("emit-c", "H"), ("emit-asm", "INC")):
        target = build / f"KEYB_UK.{suffix}"
        result = subprocess.run([sys.executable, str(GEN), kind, "--layout", "UK",
                                 "--output", str(target)], cwd=ROOT, check=False)
        if result.returncode:
            return result.returncode
        outputs.append(target)
    c_bytes = bytes_from_c(outputs[0].read_text(encoding="utf-8"))
    asm_bytes = bytes_from_asm(outputs[1].read_text(encoding="utf-8"))
    if len(asm_bytes) != 7 * 128 or not c_bytes:
        print("UK fallback emitters produced unexpected byte counts", file=sys.stderr)
        return 1
    # emit-c appends the dead-key trailer; the assembler file stops after the
    # seven tables that the nine kernel pointers reference.
    if c_bytes[:len(asm_bytes)] != asm_bytes:
        print("UK fallback C and assembler emitters disagree", file=sys.stderr)
        return 1
    english = (build / "locale-en" / "MAGIC.RAM").read_bytes()
    german = (build / "locale-de" / "MAGIC.RAM").read_bytes()
    if asm_bytes not in english:
        print("English kernel does not carry the canonical UK fallback", file=sys.stderr)
        return 1
    if asm_bytes in german:
        print("German kernel unexpectedly carries the UK fallback", file=sys.stderr)
        return 1
    print("kernel regions, UK fallback emitters and embedded tables verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
