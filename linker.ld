ENTRY(_start)

SECTIONS {
    . = 1M;                     /* load kernel at 1MB */

    kernel_start = .; 

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

    kernel_end = .; 
}