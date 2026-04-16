
MAGIC    equ 0xe85250d6
ARCH     equ 0           ; x86
HDRLEN   equ header_end - header_start
CHECKSUM equ -(MAGIC + ARCH + HDRLEN)

[BITS 32]
global _start 

section .multiboot
align 8           ; Multiboot2 requires 8-byte alignment
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
align 4096
pml4_table: resb 4096
pdpt_table: resb 4096
pd_table:   resb 4096
align 16 
stack_bottom: resb 16384
stack_top:

section .text
bits 32
_start:
    mov esp, stack_top

    ; 1. Setup Identity Paging (First 2MB)
    ; Link PML4 entry 0 to PDPT
    mov eax, pdpt_table
    or eax, 0b11 ; Present + Writable
    mov [pml4_table], eax

    ; Link PDPT entry 0 to PD
    mov eax, pd_table
    or eax, 0b11
    mov [pdpt_table], eax

    ; Link PD entry 0 to a 2MB Huge Page (starts at address 0)
    mov eax, 0b10000011 ; Present + Writable + Huge
    mov [pd_table], eax

    ; 2. Point CR3 to PML4
    mov eax, pml4_table
    mov cr3, eax

    ; 3. Enable PAE (Physical Address Extension)
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; 4. Set Long Mode bit in EFER MSR
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; 5. Enable Paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; 6. Load 64-bit GDT and Jump
    lgdt [gdt64.pointer]
    jmp gdt64.code:long_mode_start

bits 64
long_mode_start:
    ; Load NULL into data segment registers
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov rsp, stack_top
    extern kernel_main
    call kernel_main
    hlt

section .rodata
gdt64:
    dq 0 ; null descriptor
.code: equ $ - gdt64
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53) ; code descriptor
.pointer:
    dw $ - gdt64 - 1
    dd gdt64      ; Changed to dd (4 bytes) for 32-bit compatibility