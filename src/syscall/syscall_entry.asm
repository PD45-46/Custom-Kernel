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

    ; Switch to kernel stack
    ; Can't push RSP on itself safely here. Use the swapgs 
    ; trick or a per-CPU temp variable. For now, use a global 
    ; (single CPU, no userspace yet). 

    mov[user_rsp_tmp], rsp 
    mov rsp, [kernel_stack_top]

    ; Re-enable interrupts 
    ; They were initially disabled via SFMASK on entry. 

    sti 

    ; Save user registers 

    push rcx 
    push r11 
    push rbp 
    push rbx
    push r12 
    push r13 
    push r14 
    push r15 
    
    push qword [user_rsp_tmp] 


    ; Call C dispatcher 
    ; Arguments already in correct registers 
    ;   RDI = arg1, RSI = arg2, RDX = arg3, R10 = arg4
    ; 
    ; Syscall number is in RAX, pass it to the first argument 
    ; by moving it to RDI and shifting the rest. 
    ; 
    ; syscall_dispatch(number, arg1, arg2, arg3)
    ; = syscall_dispatch(rax, rdi, rsi, rdx) 

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

    call syscall_dispatch

    ; RAX now holds the return value from syscall_dispatch 
    ; leave it in RAX, sysret will return it to user space

    ; Restore registers 
    pop qword [user_rsp_tmp]  
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    pop r11
    pop rcx

    ; Restore user stack 
    mov rsp, [user_rsp_tmp] 

    ; return to user space 
    ; sysretq restores RIP from RCX, RFLAGS from R11,
    ; switches CS back to user code selector.
    ; Must disable interrupts before sysret. 
    ; Interrupts are re-enabled as part of RFLAGS restore.

    cli 
    sysretq 

section .data 
user_rsp_tmp: dq 0 

section .note.GNU-stack noalloc noexec nowrite progbits