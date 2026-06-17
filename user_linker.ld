OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)

PHDRS {
    text PT_LOAD FLAGS(5);   /* r-x */
    data PT_LOAD FLAGS(6);   /* rw- */
}

SECTIONS {
    . = 0x8001000000;

    .text   : { *(.text*)   } :text
    .rodata : { *(.rodata*) } :text

    . = ALIGN(4096);
    .data   : { *(.data*)   } :data
    .bss    : { *(.bss*) *(COMMON) } :data
}