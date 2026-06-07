
; RDI = first argument = pointer to current process_t 
; RSI = second argument = pointer to next process_t 


global context_switch 

%define PROCESS_CONTEXT_RSP 8
%define PROCESS_PAGE_TABLE 176 ; Offset of page table in process_t 

context_switch: 

    ; save current stack pointer 
    mov [rdi + PROCESS_CONTEXT_RSP], rsp 

    ; swap address space if next process has its own page table 
    mov rax, [rsi + PROCESS_PAGE_TABLE] 
    test rax, rax 
    jz .no_cr3_swap 
    mov cr3, rax

.no_cr3_swap: 

    ; load next process stack pointer 
    mov rsp, [rsi + PROCESS_CONTEXT_RSP] 
    ret 

section .note.GNU-stack noalloc noexec nowrite progbits