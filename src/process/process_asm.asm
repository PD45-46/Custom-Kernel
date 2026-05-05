
extern pic_send_eoi
global process_trampoline_fn
global process_user_trampoline_fn

process_trampoline_fn:
    ; RSP currently points to the entry function pointer pushed by process_create.
    ; No compiler prologue will touch the stack before this.
    pop  rdi                  ; grab entry_fn_ptr off the stack into rdi (callee-saved via below)
    push rdi                  ; save it — pic_send_eoi call may clobber rdi
    
    xor  edi, edi             ; arg 0: IRQ line 0 (timer)
    call pic_send_eoi         ; ACK the PIC so future IRQs can fire
    
    pop  rax                  ; restore entry_fn_ptr
    sti                       ; re-enable interrupts (critical!)
    call rax                  ; jump into the process's entry function
    
    ; If entry ever returns, halt
.dead:
    cli
    hlt
    jmp .dead


process_user_trampoline_fn: 
    ; ACK PIC first so timer keeps going 
    xor edi, edi 
    call pic_send_eoi

    ; pop RIP and RSP that the process_create_user pushed 
    pop rcx
    pop rdx

    ; build iretq frame on current kernel stack 
    push 0x23
    push rdx
    push 0x202
    push 0x1B 
    push rcx

    ; zero gen purp registers before entering user space 
    ; to aavoid leaking kernel data 
    xor rax, rax
    xor rbx, rbx
    xor rcx, rcx
    xor rdx, rdx
    xor rsi, rsi
    xor rdi, rdi
    xor r8,  r8
    xor r9,  r9
    xor r10, r10
    xor r11, r11
    xor r12, r12
    xor r13, r13
    xor r14, r14
    xor r15, r15
    xor rbp, rbp

    ; drop into ring 3 
    iretq 
section .note.GNU-stack noalloc noexec nowrite progbits