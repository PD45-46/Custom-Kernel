; syscall_entry - raw entry point for the syscall instruction 
;
; CPU state on arrival 
;   RAX = syscall number
;   RDI = arg1, RSI = arg2, RDX = arg3, R10 = arg4
;   RCX = saved user RIP  (must be preserved for sysret)
;   R11 = saved RFLAGS    (must be preserved for sysret)
;   RSP = still pointing at USER stack (dangerous!)
;   CS  = kernel code selector
;   interrupts are DISABLED (SFMASK cleared IF)
;
;
; Order of operations 
;   1. Switch to kernel stack 
;   2. Save all user registers 
;   3. Call C dispatcher with syscall number and args
;   4. Restore registers
;   5. sysretq back to user space

global syscall_entry
extern syscall_dispatch 
extern kernel_stack_top 

syscall_entry: 
    mov [user_rsp_tmp], rsp 
    mov rsp, [kernel_stack_top]
    ; NO sti — keep interrupts disabled throughout

    push rcx 
    push r11 
    push rbp 
    push rbx
    push r12 
    push r13 
    push r14 
    push r15 
    push qword [user_rsp_tmp]

    push rdi 
    push rsi 
    push rdx
    mov rcx, rdx
    mov rdx, rsi 
    mov rsi, rdi 
    mov rdi, rax
    pop rax
    pop rax
    pop rax
    call syscall_dispatch    ; IF=0 throughout — context_switch is safe

    pop qword [user_rsp_tmp]  
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    pop r11
    pop rcx

    mov rsp, [user_rsp_tmp] 
    db 0x48, 0x0F, 0x07     ; sysretq — 48=REX.W, 0F 07=SYSRET opcode

section .data 
user_rsp_tmp: dq 0 
section .note.GNU-stack noalloc noexec nowrite progbits