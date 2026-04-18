
extern pic_send_eoi
global process_trampoline_fn

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