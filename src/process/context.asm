
; RDI = first argument = pointer to current process_t 
; RSI = second argument = pointer to next process_t 


global context_switch 

%define PROCESS_CONTEXT_RSP 8

context_switch: 

    ; Save current process 
    ; Still on the current process's stack. Only need to save rsp because 
    ; all other registers were already pushed onto the stack by the IRQ stub 
    ; in isr.asm before this function was called.

    mov [rdi + PROCESS_CONTEXT_RSP], rsp 

    ; Load next process
    ; Switching to the next process's stack by loading in its saved RSP. 
    ; After this instruction we're working on a completely different stack 
    ; --- the one that belongs to the next process. 

    mov rsp, [rsi + PROCESS_CONTEXT_RSP] 

    ; Pop RIP off the new stack and jumps there. 
    ; For a process that is new: jumps to the 'trampoline' in process_create 
    ; For pre-existing process: resumes where it was left off. 

    ret

section .note.GNU-stack noalloc noexec nowrite progbits