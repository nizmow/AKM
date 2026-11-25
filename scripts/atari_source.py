#!/usr/bin/env python3
"""Check or convert Atari ST text while preserving runtime string bytes.

Non-ASCII bytes in code and comments become UTF-8. Non-ASCII bytes inside C
string/character literals become explicit octal escapes so the compiled bytes
remain Atari bytes. Binary resources and unknown text are never rewritten.
"""

import argparse
import codecs
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "NON-TOS/atari-utf8-conversion"))
import atari_text

TEXT_SUFFIXES = {".c", ".h", ".s", ".asm"}


def decode_atari(data):
    table = atari_text.character_table()
    return codecs.charmap_decode(data, "strict", table)[0]


def is_utf8(data):
    try:
        data.decode("utf-8")
        return True
    except UnicodeDecodeError:
        return False


def convert_bytes(data):
    table = atari_text.character_table()
    output = bytearray()
    state = "code"
    quote = None
    index = 0
    while index < len(data):
        byte = data[index]
        nxt = data[index + 1] if index + 1 < len(data) else None
        if state == "code":
            if byte == ord("/") and nxt == ord("/"):
                state, output = "line", output + b"//"
                index += 2
            elif byte == ord("/") and nxt == ord("*"):
                state, output = "block", output + b"/*"
                index += 2
            elif byte == ord('"'):
                state, quote, output = "literal", byte, output + bytes([byte])
                index += 1
            elif byte == ord("'"):
                state, quote, output = "literal", byte, output + bytes([byte])
                index += 1
            elif byte < 128:
                output.append(byte)
                index += 1
            else:
                output.extend(table[byte].encode("utf-8"))
                index += 1
        elif state == "line":
            if byte in (10, 13):
                state = "code"
                output.append(byte)
            elif byte < 128:
                output.append(byte)
            else:
                output.extend(table[byte].encode("utf-8"))
            index += 1
        elif state == "block":
            if byte == ord("*") and nxt == ord("/"):
                state, output = "code", output + b"*/"
                index += 2
            elif byte < 128:
                output.append(byte)
                index += 1
            else:
                output.extend(table[byte].encode("utf-8"))
                index += 1
        else:
            if byte == ord("\\"):
                output.append(byte)
                index += 1
                if index < len(data):
                    output.append(data[index])
                    index += 1
            elif byte == quote:
                state = "code"
                output.append(byte)
                index += 1
            elif byte < 128:
                output.append(byte)
                index += 1
            else:
                output.extend(f"\\{byte:03o}".encode("ascii"))
                index += 1
    return bytes(output)


def paths(arguments):
    for argument in arguments:
        path = Path(argument)
        if path.is_dir():
            for candidate in path.rglob("*"):
                if candidate.is_file() and candidate.suffix.lower() in TEXT_SUFFIXES:
                    yield candidate
        elif path.is_file() and path.suffix.lower() in TEXT_SUFFIXES:
            yield path
        else:
            raise SystemExit(f"{argument}: not a supported text file or directory")


def status(path):
    data = path.read_bytes()
    if is_utf8(data):
        return "utf8"
    try:
        decode_atari(data)
        return "atari"
    except (UnicodeError, ValueError):
        return "invalid"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    check = subparsers.add_parser("check", help="report text encoding")
    check.add_argument("paths", nargs="+")
    convert = subparsers.add_parser("convert", help="rewrite Atari text as UTF-8")
    convert.add_argument("paths", nargs="+")
    convert.add_argument("--write", action="store_true",
                         help="modify files; without this option only report")
    args = parser.parse_args()

    if args.command == "check":
        for path in paths(args.paths):
            print(f"{status(path):8} {path}")
        return 0

    failures = 0
    for path in paths(args.paths):
        state = status(path)
        if state == "utf8":
            continue
        if state != "atari":
            print(f"skip invalid {path}", file=sys.stderr)
            failures += 1
            continue
        output = convert_bytes(path.read_bytes())
        if args.write:
            path.write_bytes(output)
            print(f"converted {path}")
        else:
            try:
                output.decode("utf-8")
            except UnicodeDecodeError:
                print(f"conversion failed {path}", file=sys.stderr)
                failures += 1
                continue
            print(f"would convert {path}")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
