#!/usr/bin/env python3
"""Package a linked, relocatable m68k ELF as a classic CPX module.

The output is a 512-byte CPX header followed by a classic TOS executable
(program header, text, data, BSS size and TOS relocations), matching the
format loaded by XCONTROL/COPS.
"""
import argparse
import struct
from pathlib import Path


def u(data, offset, fmt):
    return struct.unpack_from(">" + fmt, data, offset)


def cstring(data, offset):
    end = data.index(0, offset)
    return data[offset:end].decode("ascii")


def package(data, header):
    if data[:7] != b"\x7fELF\x01\x02\x01":
        raise ValueError("expected big-endian ELF32")
    (kind, machine, _, _, _, shoff, _, _, _, _,
     shentsize, shnum, shstr) = u(data, 16, "HHIIIIIHHHHHH")
    if (kind, machine, shentsize) != (2, 4, 40):
        raise ValueError("expected executable m68k ELF32")
    sections = [u(data, shoff + i * shentsize, "10I") for i in range(shnum)]
    names_sec = sections[shstr]
    names = data[names_sec[4]:names_sec[4] + names_sec[5]]
    named = {cstring(names, s[0]): (i, s) for i, s in enumerate(sections)}
    text, initialized, bss = (named[n][1] for n in (".text", ".data", ".bss"))
    if text[3] != 0 or initialized[3] != text[5] or bss[3] != text[5] + initialized[5]:
        raise ValueError("TEXT, DATA, and BSS must be contiguous from address zero")
    if text[1] != 1 or initialized[1] != 1 or bss[1] != 8:
        raise ValueError("expected PROGBITS TEXT/DATA and NOBITS BSS")
    loaded = bytearray(data[text[4]:text[4] + text[5]] +
                       data[initialized[4]:initialized[4] + initialized[5]])
    if len(loaded) != text[5] + initialized[5]:
        raise ValueError("truncated loaded payload")
    for name, (_, section) in named.items():
        if section[2] & 2 and name not in (".text", ".data", ".bss") and section[5]:
            raise ValueError("unexpected allocated section: " + name)

    symtabs = {}
    for index, section in enumerate(sections):
        if section[1] != 2:
            continue
        if section[9] != 16:
            raise ValueError("unexpected ELF symbol size")
        symtabs[index] = [u(data, off, "IIIBBH")
                          for off in range(section[4], section[4] + section[5], 16)]

    offsets = set()
    for rel in sections:
        if rel[1] not in (4, 9) or not sections[rel[7]][2] & 2:
            continue
        table = symtabs.get(rel[6])
        size = 12 if rel[1] == 4 else 8
        if table is None or rel[9] != size:
            raise ValueError("invalid relocation table")
        for off in range(rel[4], rel[4] + rel[5], size):
            location, info = u(data, off, "II")
            typ, symbol = info & 255, table[info >> 8]
            if typ == 0:
                continue
            if typ in (4, 5, 6):
                if symbol[5] == 0 or symbol[5] >= len(sections) or \
                        not sections[symbol[5]][2] & 2:
                    raise ValueError("PC-relative relocation targets nonloaded storage")
                continue
            if typ not in (1, 2, 3):
                raise ValueError("unsupported m68k relocation: " + str(typ))
            if symbol[5] == 0xfff1:
                continue
            if symbol[5] == 0 or symbol[5] >= len(sections):
                raise ValueError("unresolved relocation in loaded image")
            if not sections[symbol[5]][2] & 2:
                raise ValueError("movable relocation targets nonloaded storage")
            if typ != 1:
                raise ValueError("movable reference requires a 32-bit absolute relocation")
            if location & 1 or location + 4 > len(loaded) or location in offsets:
                raise ValueError("invalid or duplicate relocation location")
            offsets.add(location)

    locs = sorted(offsets)
    relocs = bytearray()
    if locs:
        relocs += struct.pack(">I", locs[0])
        previous = locs[0]
        for location in locs[1:]:
            delta = location - previous
            while delta > 254:
                relocs.append(1)
                delta -= 254
            relocs.append(delta)
            previous = location
        relocs.append(0)
    program = struct.pack(">H6IH", 0x601a, text[5], initialized[5], bss[5],
                          0, 0, 0, 0) + loaded + relocs
    if len(header) != 512:
        raise ValueError("CPX header must be 512 bytes")
    return header + program


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf", type=Path)
    parser.add_argument("header", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    header = args.header.read_bytes()
    image = package(args.elf.read_bytes(), bytearray(header))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(image)
    print(f"{args.output}: {len(image)} bytes")


if __name__ == "__main__":
    main()
