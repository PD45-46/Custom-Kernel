PHDRS {
    text PT_LOAD FLAGS(5);    /* r-x */
    data PT_LOAD FLAGS(6);    /* rw- */
}

SECTIONS {
    . = 1M;

    .text : { 
        KEEP(*(.multiboot))
        *(.text)
    } :text

    .rodata : { *(.rodata) } :data

    .data : { *(.data) } :data

    .bss : {
        *(COMMON)
        *(.bss)
    } :data
}