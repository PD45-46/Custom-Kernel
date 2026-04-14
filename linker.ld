ENTRY(_start)

SECTIONS {
    . = 1M;                     /* load kernel at 1MB */

    .text : {
        KEEP(*(.multiboot))     /* multiboot header must be first */
        *(.text)
    }

    .rodata : { *(.rodata) }

    .data : { *(.data) }

    .bss : {
        *(COMMON)
        *(.bss)
    }
}