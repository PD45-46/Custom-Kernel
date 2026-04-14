
MAGIC    equ 0xe85250d6
ARCH     equ 0           ; x86
HDRLEN   equ header_end - header_start
CHECKSUM equ -(MAGIC + ARCH + HDRLEN)

section .multiboot
header_start:
    dd MAGIC
    dd ARCH
    dd HDRLEN
    dd CHECKSUM
    ; end tag
    dw 0
    dw 0
    dd 8
header_end:

section .bss
align 16
stack_bottom:
    resb 16384          ; 16KB stack
stack_top:

section .text
global _start
extern kernel_main

_start:
    mov rsp, stack_top  ; set up stack
    call kernel_main    ; jump to C
    hlt                 ; should never reach here