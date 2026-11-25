OUTPUT_FORMAT("elf32-m68k")
OUTPUT_ARCH(m68k)
ENTRY(_start)

SECTIONS
{
  . = 0;
  .text :
  {
    *magibios.o(.text .text.* CODE)
    *(.text .text.* CODE)
    *(.rodata .rodata.*)
    . = ALIGN(4);
  }
  .data :
  {
    *(EXCLUDE_FILE (*ende.o) .data .data.* DATA)
    /* MAGXBOOT copies TEXT+DATA only: initialized storage includes GCC BSS. */
    BYTE(0)
    *(.bss .bss.* BSS)
    *(COMMON)
    . = ALIGN(8);
    *ende.o(.data .data.* DATA)
  }
  /DISCARD/ : { *(.comment .note.* .eh_frame .eh_frame_hdr) }
}
ASSERT(_start == 0, "BIOS entry must be the first byte of MAGIC.RAM")
ASSERT(_ende == ADDR(.data) + SIZEOF(.data), "ENDE must terminate the loaded kernel")
ASSERT((SIZEOF(.text) + SIZEOF(.data)) % 8 == 0, "MAGXBOOT requires aligned copy length")
