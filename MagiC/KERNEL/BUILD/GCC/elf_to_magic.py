"""Package the zero-based kernel ELF as the classic TOS image MAGXBOOT loads.

The ELF must retain relocations (--emit-relocs). Only absolute 32-bit references
to loaded sections need TOS base relocation; fixed low-memory symbols must not
be relocated. BSS is materialized by kernel.ld because MAGXBOOT never clears it.
"""
import argparse
import struct
from pathlib import Path


def unpack(data, offset, fmt):
    return struct.unpack_from(">" + fmt, data, offset)


def string(data, offset):
    return data[offset:data.index(0, offset)].decode("ascii")


def package(data):
    if data[:7] != b"\x7fELF\x01\x02\x01":
        raise ValueError("expected a big-endian ELF32 kernel")
    header = unpack(data, 16, "HHIIIIIHHHHHH")
    kind, machine, _, entry, _, shoff, _, _, _, _, shsize, shnum, shstr = header
    if (kind, machine, entry, shsize) != (2, 4, 0, 40):
        raise ValueError("expected zero-entry executable m68k ELF with 40-byte section headers")
    sections = [unpack(data, shoff + i * shsize, "10I") for i in range(shnum)]
    names_section = sections[shstr]
    names = data[names_section[4]:names_section[4] + names_section[5]]
    named = {string(names, s[0]): s for s in sections}
    text, initialized = named[".text"], named[".data"]
    if text[3] != 0 or initialized[3] != text[5]:
        raise ValueError("TEXT and DATA must form one contiguous zero-based image")
    loaded = bytearray(data[text[4]:text[4] + text[5]])
    loaded += data[initialized[4]:initialized[4] + initialized[5]]
    if len(loaded) != text[5] + initialized[5] or len(loaded) % 8:
        raise ValueError("truncated or unaligned kernel payload")
    for name, section in named.items():
        if section[2] & 2 and name not in (".text", ".data") and section[5]:
            raise ValueError("unexpected allocated section: " + name)
    if text[1] != 1 or initialized[1] != 1:
        raise ValueError("all loaded storage, including BSS, must be PROGBITS")

    symbols = {}
    symbol_tables = {}
    for i, section in enumerate(sections):
        if section[1] != 2:
            continue
        strings_section = sections[section[6]]
        strings = data[strings_section[4]:strings_section[4] + strings_section[5]]
        if section[9] != 16:
            raise ValueError("unexpected symbol size")
        table = [unpack(data, offset, "IIIBBH")
                 for offset in range(section[4], section[4] + section[5], 16)]
        symbol_tables[i] = table
        symbols.update((string(strings, s[0]), s[1]) for s in table if s[0])
    if symbols.get("_start") != 0 or symbols.get("_ende") != len(loaded):
        raise ValueError("kernel entry/end symbols do not match the loaded image")

    relocation_offsets = set()
    for section in sections:
        if section[1] not in (4, 9) or not sections[section[7]][2] & 2:
            continue
        table = symbol_tables[section[6]]
        size = 12 if section[1] == 4 else 8
        if section[9] != size:
            raise ValueError("unexpected relocation size")
        for offset in range(section[4], section[4] + section[5], size):
            location, info = unpack(data, offset, "II")
            symbol = table[info >> 8]
            relocation_type = info & 255
            if relocation_type == 0:
                continue
            if relocation_type in (4, 5, 6):  # PC-relative references move together.
                if (symbol[5] == 0 or symbol[5] >= len(sections)
                        or not sections[symbol[5]][2] & 2):
                    raise ValueError("PC-relative reference does not target loaded storage")
                continue
            if relocation_type not in (1, 2, 3):
                raise ValueError("unsupported m68k relocation: " + str(relocation_type))
            if symbol[5] == 0xfff1:  # Absolute low-memory or numeric constant.
                continue
            if symbol[5] == 0 or symbol[5] >= len(sections):
                raise ValueError("unresolved relocation in the loaded kernel")
            if not sections[symbol[5]][2] & 2:
                raise ValueError("movable address does not target loaded storage")
            if relocation_type != 1:
                raise ValueError("movable address uses a non-32-bit absolute relocation")
            if location & 1 or location < 4 or location + 4 > len(loaded):
                raise ValueError("invalid TOS relocation location")
            if location in relocation_offsets:
                raise ValueError("multiple relocations for one kernel address")
            relocation_offsets.add(location)

    # Check the exact ROM-style header and AES block inspected by MAGXBOOT.S.
    if loaded[0] != 0x60:
        raise ValueError("kernel must begin with its BIOS branch, not an ELF startup wrapper")
    aes = unpack(loaded, 0x14, "I")[0]
    if aes + 16 > len(loaded) or loaded[aes:aes + 4] != b"\x87\x65\x43\x21" or loaded[aes + 12:aes + 16] != b"MAGX":
        raise ValueError("missing MagiC AES signature")
    load_base = unpack(loaded, aes + 4, "I")[0]
    if not 0 < load_base < 0x1000000:
        raise ValueError("invalid absolute AES variable boundary")
    if 0x0c in relocation_offsets or aes + 4 in relocation_offsets:
        raise ValueError("fixed low-memory variable boundaries must not be relocated")
    if not {4, 8, 16, 20}.issubset(relocation_offsets):
        raise ValueError("BIOS header pointers are missing base relocations")
    relocated = bytearray(loaded)
    for offset in relocation_offsets:
        value = (unpack(relocated, offset, "I")[0] + load_base) & 0xffffffff
        struct.pack_into(">I", relocated, offset, value)
    if unpack(relocated, 8, "I")[0] != load_base or unpack(relocated, 20, "I")[0] != aes + load_base:
        raise ValueError("kernel header does not relocate to the loader's chosen base")

    locations = sorted(relocation_offsets)
    if not locations:
        raise ValueError("a relocatable kernel requires a relocation table")
    relocation_table = bytearray(struct.pack(">I", locations[0]))
    previous = locations[0]
    for location in locations[1:]:
        delta = location - previous
        if delta < 4:
            raise ValueError("overlapping relocation fields")
        while delta > 254:
            relocation_table.append(1)
            delta -= 254
        relocation_table.append(delta)
        previous = location
    relocation_table.append(0)
    tos_header = struct.pack(">H6IH", 0x601a, text[5], initialized[5], 0, 0, 0, 0, 0)
    return tos_header + loaded + relocation_table, len(locations), load_base


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    image, relocations, base = package(args.elf.read_bytes())
    args.output.write_bytes(image)
    print(f"{args.output}: {len(image)} bytes, {relocations} relocations, ST-RAM load base 0x{base:x}")
