ENTRY(_start)

PHDRS {
    text PT_LOAD FLAGS(5);    /* r-x */
    data PT_LOAD FLAGS(6);    /* rw- */
}

SECTIONS {
    . = 1M;                     /* load kernel at 1MB */

    PROVIDE(kernel_start = .);

    .text : {
        KEEP(*(.multiboot))
        *(.text)
    } :text

    .rodata : { *(.rodata) } :text

    .data : { *(.data) } :data

    .bss : {
        *(COMMON)
        *(.bss)
    } :data

    PROVIDE(kernel_end = .);

    . = ALIGN(4096); 
    _kernel_end = .; 
}