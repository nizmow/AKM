OUTPUT_FORMAT("elf32-m68k")
OUTPUT_ARCH(m68k)
ENTRY(slb_header)
SECTIONS
{
  . = 0;
  .text : {
    KEEP(*(.slb_header))
    *(.text .text.*)
    *(.rodata .rodata.*)
    . = ALIGN(4);
  }
  .data : {
    *(.data .data.*)
    *(.sdata .sdata.*)
    . = ALIGN(4);
    __slb_init_array_start = .;
    KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
    KEEP(*(.init_array))
    . = ALIGN(4);
    __slb_init_array_end = .;
    . = ALIGN(4);
    __slb_fini_array_start = .;
    KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*)))
    KEEP(*(.fini_array))
    . = ALIGN(4);
    __slb_fini_array_end = .;
  }
  .bss (NOLOAD) : {
    *(.bss .bss.*)
    *(.sbss .sbss.*)
    *(COMMON)
    . = ALIGN(4);
  }
  /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
