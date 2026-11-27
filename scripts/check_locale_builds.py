#!/usr/bin/env python3
"""Public locale build checks: kernel regions and UK fallback emitter agreement.

Only repository inputs are used; no MagiC image or proprietary ROM is needed.
Both the nine write slots and the seven distinct table bodies are compared, so
a swapped pointer slot cannot pass while the table bytes stay unchanged.
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
SLOTS = 9
TABLE_BYTES = 128


def parse_c(text: str) -> tuple[dict[str, bytes], list[str]]:
    clean = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    clean = re.sub(r"//[^\n]*", "", clean)
    bodies = {
        match.group(1): bytes(int(value, 16) for value in
                              re.findall(r"0x([0-9a-fA-F]{2})", match.group(2)))
        for match in re.finditer(
            r"^\s*static const char tab_(\w+)\[128\] = \{(.*?)^\s*\};", clean,
            re.S | re.M)
    }
    order_block = re.search(
        r"^\s*static const char \*const tab_order\[9\] = \{(.*?)^\s*\};",
        clean, re.S | re.M)
    if order_block is None:
        raise ValueError("C header has no tab_order")
    order = []
    for entry in order_block.group(1).split(","):
        if not entry.strip():
            continue
        match = re.fullmatch(r"\s*tab_(\w+)\s*", entry)
        if match is None:
            raise ValueError(f"unexpected tab_order entry {entry!r}")
        order.append(match.group(1))
    return bodies, order


def parse_asm(text: str) -> tuple[dict[str, bytes], list[str]]:
    lines = [line for line in text.splitlines()
             if not line.lstrip().startswith(("*", ";"))]
    bodies: dict[str, bytes] = {}
    order: list[str] = []
    index = 0
    while index < len(lines):
        line = lines[index]
        label = re.fullmatch(r"tab_(\w+):", line)
        if label is None:
            index += 1
            continue
        values: list[int] = []
        index += 1
        while index < len(lines) and lines[index].startswith(" dc.b "):
            values += [int(value, 16)
                       for value in re.findall(r"\$([0-9a-fA-F]{2})", lines[index])]
            index += 1
        bodies[label.group(1)] = bytes(values)
    for number, line in enumerate(lines):
        if line != "default_keytblx:":
            continue
        index = number + 1
        while len(order) < SLOTS and index < len(lines):
            candidate = lines[index]
            index += 1
            if not candidate.strip():
                continue
            slot = re.fullmatch(r" dc\.l tab_(\w+)", candidate)
            if slot is None:
                raise ValueError(f"unexpected default pointer {candidate!r}")
            order.append(slot.group(1))
        # Everything up to the first table label must be exactly the slots.
        while index < len(lines):
            candidate = lines[index]
            index += 1
            if not candidate.strip():
                continue
            if re.fullmatch(r"tab_\w+:", candidate):
                break
            raise ValueError(f"extra data after default pointers: {candidate!r}")
        break
    return bodies, order


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
    c_bodies, c_order = parse_c(outputs[0].read_text(encoding="utf-8"))
    asm_bodies, asm_order = parse_asm(outputs[1].read_text(encoding="utf-8"))
    if len(c_order) != SLOTS or len(asm_order) != SLOTS:
        print("UK fallback does not define nine write slots", file=sys.stderr)
        return 1
    if c_order != asm_order:
        print(f"UK fallback slot order differs: C {c_order} vs asm {asm_order}",
              file=sys.stderr)
        return 1
    names = set(c_order)
    if set(asm_bodies) != names or set(c_bodies) != names:
        print("UK fallback table names differ between emitters", file=sys.stderr)
        return 1
    for name in names:
        if len(c_bodies[name]) != TABLE_BYTES or c_bodies[name] != asm_bodies[name]:
            print(f"UK fallback table {name} differs between emitters", file=sys.stderr)
            return 1
    # The include emits the distinct tables once, in definition order; the nine
    # pointers index into that contiguous block.
    block = b"".join(c_bodies[name] for name in c_bodies)
    if len(block) != 7 * TABLE_BYTES:
        print("UK fallback emitted an unexpected table count", file=sys.stderr)
        return 1
    english = (build / "locale-en" / "MAGIC.RAM").read_bytes()
    german = (build / "locale-de" / "MAGIC.RAM").read_bytes()
    if block not in english:
        print("English kernel does not carry the canonical UK fallback", file=sys.stderr)
        return 1
    if block in german:
        print("German kernel unexpectedly carries the UK fallback", file=sys.stderr)
        return 1
    print("kernel regions, UK slot order, table bodies and embedded fallback verified")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
