#!/usr/bin/env python3
"""Emit keyboard tables from the canonical keymap data.

``base/keyb-sys/keymaps.json`` is the single source of truth for the six
KEYB_* table generators and for the UK fallback table compiled into the BIOS.
Every table is a list of byte values (0..255); no character encoding is
involved, so the generated output is stable across host locales.

Usage:
    gen_keymaps.py emit-c   --layout UK --output KEYB_UK.H
    gen_keymaps.py emit-asm --layout UK --output KEYB_UK.INC
    gen_keymaps.py list
"""
import argparse
import json
import pathlib
import sys

DATA = pathlib.Path(__file__).resolve().parent.parent / "keymaps.json"


def fail(message):
    print(f"gen_keymaps.py: {message}", file=sys.stderr)
    raise SystemExit(1)


def load(path):
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except OSError as exc:
        fail(f"cannot read {path}: {exc}")
    except json.JSONDecodeError as exc:
        fail(f"{path}: {exc}")
    if data.get("format") != 1:
        fail(f"{path}: unsupported format {data.get('format')!r}")
    tables = data.get("table_names")
    if not isinstance(tables, list) or not tables:
        fail(f"{path}: table_names missing")
    for table in tables:
        if table == "dead":
            fail(f"{path}: 'dead' must not be listed in table_names")
    write_order = data.get("write_order")
    if not isinstance(write_order, list) or len(write_order) != 9:
        fail(f"{path}: write_order must contain nine table names")
    for table in write_order:
        if table not in tables:
            fail(f"{path}: write_order references unknown table {table!r}")
    layouts = data.get("layouts")
    if not isinstance(layouts, dict) or not layouts:
        fail(f"{path}: layouts missing")
    for name, layout in layouts.items():
        values = layout.get("tables", {})
        for table in tables:
            entries = values.get(table)
            if not isinstance(entries, list) or len(entries) != 128:
                fail(f"{path}: layout {name} table {table} is not 128 bytes")
            if any(not isinstance(v, int) or not 0 <= v <= 255 for v in entries):
                fail(f"{path}: layout {name} table {table} has non-byte values")
        dead = layout.get("dead")
        if not isinstance(dead, list) or not dead:
            fail(f"{path}: layout {name} dead trailer missing")
        if any(not isinstance(v, int) or not 0 <= v <= 255 for v in dead):
            fail(f"{path}: layout {name} dead trailer has non-byte values")
    return data


def get_layout(data, name):
    try:
        return data["layouts"][name]
    except KeyError:
        fail(f"unknown layout {name!r} (known: {', '.join(data['layouts'])})")


def byte_lines(values, formatter, per_line=16):
    lines = []
    for start in range(0, len(values), per_line):
        chunk = values[start:start + per_line]
        lines.append(" " + ",".join(formatter(v) for v in chunk) +
                     ("," if start + per_line < len(values) else ""))
    return lines


def emit_c(data, name):
    layout = get_layout(data, name)
    guard = f"KEYB_{name}_H"
    lines = [
        f"/* Generated from {data.get('source_path', 'keymaps.json')}; do not edit. */",
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
    ]
    for table in data["table_names"]:
        lines.append(f"/* {layout.get('description', name)}: {table} */")
        lines.append(f"static const char tab_{table}[128] = {{")
        lines += byte_lines(layout["tables"][table],
                            lambda v: f"0x{v:02x}")
        lines += ["};", ""]
    dead = layout["dead"]
    lines.append(f"/* {layout.get('description', name)}: dead-key trailer */")
    lines.append(f"static const char tab_dead[{len(dead)}] = {{")
    lines += byte_lines(dead, lambda v: f"0x{v:02x}")
    lines += ["};", "",
              f"#endif /* {guard} */", ""]
    return "\n".join(lines)


def emit_asm(data, name):
    layout = get_layout(data, name)
    lines = [
        f"* Generated from {data.get('source_path', 'keymaps.json')}; do not edit.",
        "* Nine default table pointers followed by the seven 128-byte tables.",
        "default_keytblx:",
    ]
    for table in data["write_order"]:
        lines.append(f" dc.l tab_{table}")
    lines.append("")
    for table in data["table_names"]:
        lines.append(f"tab_{table}:")
        values = layout["tables"][table]
        for start in range(0, len(values), 16):
            chunk = values[start:start + 16]
            lines.append(" dc.b " + ",".join(f"${v:02x}" for v in chunk))
        lines.append("")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--data", type=pathlib.Path, default=DATA,
                        help="canonical keymap JSON (default: %(default)s)")
    sub = parser.add_subparsers(dest="command", required=True)

    emit_c_parser = sub.add_parser("emit-c", help="write a C initializer header")
    emit_c_parser.add_argument("--layout", required=True)
    emit_c_parser.add_argument("--output", type=pathlib.Path, required=True)

    emit_asm_parser = sub.add_parser(
        "emit-asm", help="write the vasm source of the default table block")
    emit_asm_parser.add_argument("--layout", required=True)
    emit_asm_parser.add_argument("--output", type=pathlib.Path, required=True)

    sub.add_parser("list", help="print the available layout names")

    args = parser.parse_args()
    data = load(args.data)

    if args.command == "list":
        print("\n".join(data["layouts"]))
        return
    if args.layout not in data["layouts"]:
        fail(f"unknown layout {args.layout!r} (known: {', '.join(data['layouts'])})")

    if args.command == "emit-c":
        text = emit_c(data, args.layout)
    else:
        text = emit_asm(data, args.layout)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(text, encoding="ascii", newline="\n")


if __name__ == "__main__":
    main()
