"""Normalize MAS syntax used by the filesystem sources; keep 68k instructions intact.

Inputs are explicitly listed by assembly.mk. Includes use that flat, uppercase
namespace. Generated files live under build/atari, never over the original source.
"""
import argparse
import re
from pathlib import Path


def strip_comments(text):
    depth = 0
    for line in text.splitlines():
        output = []
        quote = None
        i = 0
        while i < len(line):
            if depth:
                if line.startswith("/*", i):
                    depth += 1
                    i += 2
                elif line.startswith("*/", i):
                    depth -= 1
                    i += 2
                    output.append(" ")
                else:
                    i += 1
            elif quote:
                output.append(line[i])
                if line[i] == quote:
                    quote = None
                i += 1
            elif line[i] == ";" or (not "".join(output).strip() and line[i] == "*"):
                output.append(line[i:])
                break
            elif (line[i] == "*" and i > 1 and line[i - 2:i].isspace()
                  and (i + 1 == len(line) or line[i + 1].isspace())
                  and not re.search(r"\b(EQU|SET)\s*$", "".join(output), re.I)):
                # MAS uses a separate comment column; preserve expression
                # multiplication and the current-PC expression EQU *.
                output.append(";" + line[i + 1:])
                break
            elif line[i] in "\"'":
                quote = line[i]
                output.append(line[i])
                i += 1
            elif line.startswith("/*", i):
                depth = 1
                i += 2
            else:
                output.append(line[i])
                i += 1
        yield "".join(output)
    if depth:
        raise ValueError("unterminated MAS block comment")


def replace_symbols(line, replacements):
    # MAS strings do not use C backslash escapes; do not rewrite their contents.
    chunks = re.split(r'''("[^"]*"|'[^']*'|;.*$)''', line)
    for i in range(0, len(chunks), 2):
        chunks[i] = re.sub(r"\b[A-Za-z_][A-Za-z_0-9]*\b",
                           lambda m: replacements.get(m[0], m[0]), chunks[i])
        chunks[i] = re.sub(r"\b0x([0-9a-fA-F]+)\b", lambda m: "$" + m[1], chunks[i])
        # MAS uses unary ! for bitwise complement; vasm uses ~.
        chunks[i] = re.sub(r"!(?!=)", "~", chunks[i])
        chunks[i] = re.sub(r"\s*,\s*", ",", chunks[i])
        chunks[i] = re.sub(r"\(\s*sp\s*\)", "(a7)", chunks[i], flags=re.I)
        # MAS size hints on immediates are not part of the expression's value.
        # Otherwise vasm drops the "+size" after "#symbol.l+size".
        chunks[i] = re.sub(
            r"#[^,]*",
            lambda m: re.sub(r"(?<=[\w)])\.[wl]\b", "", m[0], flags=re.I),
            chunks[i])
    return "".join(chunks)


def normalize(text, sections=()):
    replacements = {}
    blocks = []
    directives = ("equ|dc|dcb|ds|even|text|data|bss|org|include|macro|endm|"
                  "offset|import|export|module|endmod|end|rept|endr|local")
    for line in strip_comments(text):
        line = re.sub(r"^(\s*(?:\w+\s+)?)\.(" + directives + r")(?=[\s.]|$)",
                      lambda m: m[1] + m[2], line, flags=re.I)
        module = re.fullmatch(r"\s*MODULE\s+(\w+)\s*(?:;.*)?", line, re.I)
        if module:
            yield module[1] + ":"
            continue
        if re.fullmatch(r"\s*ENDMOD\s*(?:;.*)?", line, re.I):
            yield ""
            continue
        macro = re.match(r"\s*MACRO\s+(\w+)(.*)$", line, re.I)
        if macro:
            if "macro" in blocks:
                raise ValueError("nested MAS macro definition")
            args = [s for s in re.split(r"[\s,]+", macro[2].split(";", 1)[0].strip()) if s]
            if len(args) > 9:
                raise ValueError("more than nine MAS macro parameters")
            replacements = {name: "\\" + str(i + 1) for i, name in enumerate(args)}
            blocks.append("macro")
            yield "     macro " + macro[1]
            continue
        local = re.match(r"\s*LOCAL\s+(.+)$", line, re.I)
        if local and "macro" in blocks:
            for name in local[1].split(";", 1)[0].replace(",", " ").split():
                replacements[name] = name + "\\@"
            yield ""
            continue
        if re.match(r"\s*(?:\w+:\s*)?REPT\s+", line, re.I):
            blocks.append("repeat")
        if re.fullmatch(r"\s*(ENDM|ENDR)\s*(?:;.*)?", line, re.I):
            if blocks:
                block = blocks.pop()
                if block == "repeat":
                    line = "     endr"
                else:
                    replacements = {}
            else:
                raise ValueError("MAS block end without MACRO or REPT")
        else:
            line = replace_symbols(line, replacements)
        include = re.match(r'\s*INCLUDE\s+"([^"]+)"', line, re.I)
        if include:
            name = include[1].replace("\\", "/").rsplit("/", 1)[-1].upper()
            line = '     include "' + name + '"'
        elif re.fullmatch(r"\s*OFFSET(?:\s+0)?\s*(?:;.*)?", line, re.I):
            # VASM reuses its offset section; MAS starts a fresh structure.
            yield "     offset 0"
            line = "     org 0"
        elif re.fullmatch(r"\s*TEXT\s*(?:;.*)?", line, re.I):
            line = "     section .text,code"
        elif re.fullmatch(r"\s*DATA\s*(?:;.*)?", line, re.I):
            line = "     section .data,data"
        elif re.fullmatch(r"\s*BSS\s*(?:;.*)?", line, re.I):
            line = "     section .bss,bss"
        elif re.fullmatch(r"\s*SUPER\s*(?:;.*)?", line, re.I):
            # MAS privilege-checking mode, not an instruction; vasm allows it.
            line = ""
        elif re.match(r"\s*ALIGN\s+", line, re.I):
            # MAS ALIGN takes a byte count; VASM ALIGN takes its log2.
            code, sep, comment = line.partition(";")
            expression = re.split(r"\s+", code.strip(), maxsplit=1)[1].strip()
            try:
                value = int(expression.replace("$", "0x"), 0)
            except ValueError:
                raise ValueError(f"ALIGN needs a literal, got {expression!r}")
            if value <= 0 or value & (value - 1):
                raise ValueError(f"ALIGN needs a power of two, got {expression!r}")
            line = f"     align {value.bit_length() - 1}" + sep + comment
        elif re.match(r"\s*IFF\s+", line, re.I):
            line = re.sub(r"\bIFF\b", "ifeq", line, count=1, flags=re.I)
        elif re.match(r"\s*IF\s+", line, re.I):
            code, sep, comment = line.partition(";")
            expression = re.split(r"\s+", code.strip(), maxsplit=1)[1]
            line = "     if " + "".join(expression.split()) + (sep + comment)
        elif re.fullmatch(r"\s*ENDC\s*(?:;.*)?", line, re.I):
            line = "     endif"
        if re.match(r"\s+\w+\s+(EQU|SET)\b", line, re.I):
            line = line.lstrip()
        imported = re.match(r"\s*(IMPORT|EXPORT)\s+(.+)$", line, re.I)
        if imported:
            line = "     " + ("XREF" if imported[1].upper() == "IMPORT" else "XDEF") + " " + imported[2]
        if re.match(r"^(XDEF|XREF|ENDM|ENDR|REPT|ENDIF|ELSE|IF\w*|"
                    r"DC\.[BWL]|DCB\.[BWL]|DS\.[BWL]|CLR\.[BWL]|EVEN|END)\b", line, re.I):
            line = "     " + line
        label = re.match(r"^(\w+):", line)
        if label and label[1] in sections:
            yield "     section .text." + label[1] + ",code"
        yield line
    if blocks:
        raise ValueError("unterminated MAS block: " + blocks[-1])


def encode_atari(line, mapping):
    chunks = re.split(r'''("[^"]*"|'[^']*'|;.*$)''', line)
    encoded = []
    for chunk in chunks:
        if chunk.startswith(("'", '"')):
            encoded.append(bytes(ord(c) if ord(c) < 128 else mapping[c] for c in chunk))
        else:
            encoded.append(chunk.encode("utf-8"))
    return b"".join(encoded)


def atari_mapping():
    # Reuse the repository's canonical conversion table, not a host code page.
    source = Path(__file__).resolve().parents[1] / "NON-TOS/atari-utf8-conversion/atari-utf8.c"
    entries = re.findall(r'/\*\s*#define\s+\w+\s+(0x[0-9a-fA-F]+)\s*\*/\s*"([^"]*)"',
                         source.read_text(encoding="utf-8"))
    return {character: int(value, 16) for value, character in entries if len(character) == 1}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sections", default="", help="routine labels starting independent ELF sections")
    parser.add_argument("--atari-strings", action="store_true", help="encode UTF-8 string literals as Atari ST bytes")
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    source, destination = args.source, args.destination
    data = source.read_bytes()
    legacy = False
    try:
        text = data.decode("utf-8")
    except UnicodeDecodeError:
        legacy = True
        text = data.decode("latin1")
    destination.parent.mkdir(parents=True, exist_ok=True)
    lines = normalize(text, set(args.sections.split(",")))
    if args.atari_strings and not legacy:
        mapping = atari_mapping()
        destination.write_bytes(b"\n".join(encode_atari(line, mapping) for line in lines) + b"\n")
    else:
        destination.write_text("\n".join(lines) + "\n", encoding="latin1" if legacy else "utf-8")
