global set_gs_base
set_gs_base:
    mov  rax, rdi       ; rax = full 64-bit address
    mov  rdx, rdi       ; rdx = full 64-bit address
    shr  rdx, 32        ; rdx = high 32 bits (what edx needs)
    ; rax still has full address, so eax = low 32 bits (correct)
    mov  ecx, 0xC0000102 ; IA32_KERNEL_GS_BASE MSR
    wrmsr               ; writes edx(high):eax(low) = correct address
    ret

section .note.GNU-stack noalloc noexec nowrite progbits