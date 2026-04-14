; gdt_flush - loads the GDT and reloads segment registers
; argument: RDI = address of gdt_ptr (System V AMD64 ABI)
;
; After lgdt the CPU still has old segment register values cached.
; We do a far return to reload CS (code segment) with the kernel
; code selector (0x08 = index 1, GDT, ring 0).
; All other segment registers get loaded with the data selector
; (0x10 = index 2, GDT, ring 0).

global gdt_flush 
gdt_flush: 
    ; load GDT ptr 
    lgdt [rdi] 

    ; reload data segments
    mov ax, 0x10 
    mov ds, ax 
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; far return to reload CS
    ; push new CS then the addr to return to 
    pop rdi 
    mov rax, 0x08 
    push rax
    push rdi 
    retfq



