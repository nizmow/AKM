import importlib
import struct
import unittest


elf_to_slb = importlib.import_module("elf_to_slb")


def make_elf(*, bad_magic=False, missing_header_reloc=False, movable_type=1,
             undefined=False):
    """Construct a minimal genuine ELF32 big-endian m68k executable."""
    names = (".text", ".data", ".bss", ".rela.text", ".rela.data",
             ".symtab", ".strtab", ".shstrtab")
    shstr = b"\0" + b"".join(n.encode() + b"\0" for n in names)
    name_offsets = {}
    cursor = 1
    for name in names:
        name_offsets[name] = cursor
        cursor += len(name) + 1

    text = bytearray(512)
    for pos, value in ((0, 0x70004AFC), (4, 72), (8, 9), (12, 0),
                       (16, 86), (20, 86), (24, 86), (28, 86), (68, 0)):
        struct.pack_into(">I", text, pos, value)
    text[72:85] = b"EDITOBJC.SLB\0"
    text[86:90] = b"\x70\x00\x4e\x75"
    struct.pack_into(">i", text, 96, -10)
    data = struct.pack(">II", 520, 123)
    strtab = b"\0name\0life\0bssptr\0constant\0missing\0"
    str_offsets = {name: strtab.index(name.encode() + b"\0") for name in
                   ("name", "life", "bssptr", "constant", "missing")}

    def symbol(name, value, size, shndx):
        return struct.pack(">IIIBBH", str_offsets[name], value, size,
                           0x10, 0, shndx)

    symbols = [bytes(16), symbol("name", 72, 13, 1), symbol("life", 86, 4, 1),
               symbol("bssptr", 520, 4, 3), symbol("constant", 123, 0, 0xFFF1),
               symbol("missing", 0, 0, 0)]

    def rela(offset, symbol_index, typ):
        return struct.pack(">IIi", offset, (symbol_index << 8) | typ, 0)

    rel_text = b"".join(rela(pos, 1 if pos == 4 else 2, 1)
                        for pos in (4, 16, 20, 24, 28)
                        if not (missing_header_reloc and pos == 28))
    rel_text += rela(96, 2, 4)  # PC-relative loaded target.
    rel_data = rela(512, 3, movable_type) + rela(516, 4, 1)
    if undefined:
        rel_data = rela(512, 5, 1)

    blobs = [text, data, b"", rel_text, rel_data, b"".join(symbols),
             strtab, shstr]
    offsets = []
    image = bytearray(52)
    for blob in blobs:
        while len(image) & 3:
            image.append(0)
        offsets.append(len(image))
        image.extend(blob)
    shoff = (len(image) + 3) & ~3
    image.extend(bytes(shoff - len(image)))
    sections = [(0, 0, 0, 0, 0, 0, 0, 0, 0, 0)]
    specs = [
        (".text", 1, 6, 0, len(text), 0, 0, 4, 0),
        (".data", 1, 3, len(text), len(data), 0, 0, 4, 0),
        (".bss", 8, 3, len(text) + len(data), 4, 0, 0, 4, 0),
        (".rela.text", 4, 0, 0, len(rel_text), 6, 1, 4, 12),
        (".rela.data", 4, 0, 0, len(rel_data), 6, 2, 4, 12),
        (".symtab", 2, 0, 0, len(symbols) * 16, 7, 1, 4, 16),
        (".strtab", 3, 0, 0, len(strtab), 0, 0, 1, 0),
        (".shstrtab", 3, 0, 0, len(shstr), 0, 0, 1, 0),
    ]
    for index, (name, typ, flags, addr, size, link, info, align, entsize) in enumerate(specs):
        sections.append((name_offsets[name], typ, flags, addr, offsets[index],
                         size, link, info, align, entsize))
    for section in sections:
        image.extend(struct.pack(">10I", *section))
    ident = b"\x7fELF\x01\x02\x01" + bytes(9)
    image[:52] = ident + struct.pack(">HHIIIIIHHHHHH", 2, 4, 1, 0, 0,
                                     shoff, 0, 52, 0, 0, 40,
                                     len(sections), 8)
    if bad_magic:
        struct.pack_into(">I", image, offsets[0], 0)
    return bytes(image)


def decode_tos_relocations(stream):
    if not stream:
        return []
    location = struct.unpack_from(">I", stream)[0]
    result = [location]
    for delta in stream[4:]:
        if delta == 0:
            return result
        if delta == 1:
            location += 254
            continue
        location += delta
        result.append(location)
    raise AssertionError("unterminated TOS relocation stream")


class ElfToSlbTests(unittest.TestCase):
    def test_classic_prg_payload_and_base_relocations(self):
        image = elf_to_slb.package(make_elf())
        self.assertEqual(image[:2], b"\x60\x1a")
        self.assertEqual(struct.unpack_from(">6IH", image, 2),
                         (512, 8, 4, 0, 0, 15, 0))
        payload_end = 28 + 512 + 8
        payload = image[28:payload_end]
        self.assertEqual(payload[:4], b"\x70\x00\x4a\xfc")
        self.assertEqual(payload[72:85], b"EDITOBJC.SLB\0")
        self.assertEqual(payload[86:90], b"\x70\x00\x4e\x75")
        stream = image[payload_end:]
        self.assertEqual(stream[4:].count(1), 1)
        offsets = decode_tos_relocations(stream)
        self.assertEqual(offsets, [4, 16, 20, 24, 28, 512])
        relocated = bytearray(payload)
        base = 0x123400
        for offset in offsets:
            value = struct.unpack_from(">I", relocated, offset)[0]
            struct.pack_into(">I", relocated, offset, value + base)
        self.assertEqual(struct.unpack_from(">I", relocated, 4)[0], base + 72)
        for offset in (16, 20, 24, 28):
            self.assertEqual(struct.unpack_from(">I", relocated, offset)[0], base + 86)
        self.assertEqual(struct.unpack_from(">I", relocated, 512)[0], base + 520)
        self.assertEqual(struct.unpack_from(">I", relocated, 516)[0], 123)
        self.assertEqual(struct.unpack_from(">i", relocated, 96)[0], -10)
        self.assertEqual(struct.unpack_from(">I", relocated, 32)[0], 0)

    def test_pc_relative_reference_is_accepted_without_movable_relocation(self):
        image = elf_to_slb.package(make_elf())
        offsets = decode_tos_relocations(image[28 + 512 + 8:])
        self.assertNotIn(96, offsets)

    def test_rejects_unrepresentable_and_malformed_inputs(self):
        for kwargs in ({"movable_type": 2}, {"undefined": True},
                       {"bad_magic": True}, {"missing_header_reloc": True}):
            with self.subTest(**kwargs), self.assertRaises(ValueError):
                elf_to_slb.package(make_elf(**kwargs))


if __name__ == "__main__":
    unittest.main()
