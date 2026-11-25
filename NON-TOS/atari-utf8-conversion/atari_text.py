#!/usr/bin/env python3
"""Strict Atari ST/UTF-8 text conversion and ASCII C string literal generation."""

import argparse
import codecs
from pathlib import Path
import re
import sys


def character_table():
    # Share the historical converter's table rather than approximating with CP850.
    source = Path(__file__).with_name("atari-utf8.c").read_text(encoding="utf-8")
    table = source.split("const char *Atari2Unicode[0x80] =", 1)[1].split("};", 1)[0]
    # The table's #if 0 alternative for 0xf9 precedes its active definition.
    entries = dict((int(byte, 16), char) for byte, char in re.findall(
        r'/\*\s*#define\s+\w+\s+(0x[0-9a-fA-F]+)\s*\*/\s*"([^"]*)"', table))
    if set(entries) != set(range(128, 256)):
        raise ValueError("Atari table must define all bytes 0x80 through 0xff")
    result = "".join(chr(byte) for byte in range(128)) + "".join(
        entries[byte] for byte in range(128, 256))
    if len(result) != 256 or len(set(result)) != 256:
        raise ValueError("Atari table must map each byte to a unique character")
    return result


def c_literal(data):
    # Three octal digits cannot consume a following digit, unlike hex escapes.
    # Escape '?' too so the result is safe with C trigraph processing enabled.
    body = "".join(chr(byte) if 32 <= byte < 127 and byte not in (34, 63, 92)
                   else f"\\{byte:03o}" for byte in data)
    return ('"' + body + '"\n').encode("ascii")


def convert(data, mode):
    table = character_table()
    if mode == "decode":
        return codecs.charmap_decode(data, "strict", table)[0].encode("utf-8")
    encoded = codecs.charmap_encode(data.decode("utf-8"), "strict",
                                   codecs.charmap_build(table))[0]
    return c_literal(encoded) if mode == "c-literal" else encoded


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("decode", "encode", "c-literal"),
                        help="Atari to UTF-8, UTF-8 to Atari, or UTF-8 text to a C literal")
    parser.add_argument("source", nargs="?", default="-", help="input file (default: stdin)")
    parser.add_argument("-o", "--output", default="-", help="output file (default: stdout)")
    args = parser.parse_args()
    try:
        if args.source != "-" and args.output != "-":
            source, destination = Path(args.source), Path(args.output)
            if (source.resolve() == destination.resolve() or
                    (destination.exists() and source.samefile(destination))):
                raise ValueError("input and output must be different files")
        data = sys.stdin.buffer.read() if args.source == "-" else Path(args.source).read_bytes()
        result = convert(data, args.mode)  # Validate before opening the output.
        if args.output == "-":
            sys.stdout.buffer.write(result)
        else:
            Path(args.output).write_bytes(result)
    except (OSError, UnicodeError, ValueError) as error:
        parser.exit(1, f"{parser.prog}: {error}\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
