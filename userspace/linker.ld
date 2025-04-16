/* Tell the linker that we want an x86_64 ELF64 output file */
OUTPUT_FORMAT(elf64-x86-64)

/* We want the symbol kmain to be our entry point */
ENTRY(_start)

SECTIONS
{
    . = 0x400000;
    .text : { *(.text) }
    . = 0x500000;
    .data : { *(.data) }
    .bss : { *(.bss) }
}
