; Interrupt Service Routine stubs 
; 
; The CPU pushes: SS, RSP, RFLAGS, CS, RIP automatically. 
; For exceptions with error codes it also pushes the error code.
; For exceptions without error codes we push a dummy 0 so the
; stack layout is always the same in our C handler.
;
; After pushing everything we call a common C handler then iretq.

global isr_stub_table 

extern isr_common_handler
extern irq_common_handler

; macro for exceptions WITHOUT error code — push dummy 0
%macro ISR_NOERR 1
isr_%1:
    push qword 0        ; dummy error code
    push qword %1       ; interrupt number
    jmp isr_common_stub
%endmacro

; macro for exceptions WITH error code — CPU already pushed it
%macro ISR_ERR 1
isr_%1:
    push qword %1       ; interrupt number (error code already on stack)
    jmp isr_common_stub
%endmacro

; macro for hardware IRQs (32-47)
%macro IRQ 2
irq_%1:
    push qword 0
    push qword %2
    jmp irq_common_stub
%endmacro

; CPU exceptions 0-31
ISR_NOERR 0   ; divide by zero
ISR_NOERR 1   ; debug
ISR_NOERR 2   ; non-maskable interrupt
ISR_NOERR 3   ; breakpoint
ISR_NOERR 4   ; overflow
ISR_NOERR 5   ; bound range exceeded
ISR_NOERR 6   ; invalid opcode
ISR_NOERR 7   ; device not available
ISR_ERR   8   ; double fault
ISR_NOERR 9   ; coprocessor segment overrun
ISR_ERR   10  ; invalid TSS
ISR_ERR   11  ; segment not present
ISR_ERR   12  ; stack segment fault
ISR_ERR   13  ; general protection fault
ISR_ERR   14  ; page fault
ISR_NOERR 15
ISR_NOERR 16  ; x87 floating point
ISR_ERR   17  ; alignment check
ISR_NOERR 18  ; machine check
ISR_NOERR 19  ; SIMD floating point
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_ERR   30  ; security exception
ISR_NOERR 31

; Hardware IRQs 0-15 (mapped to IDT entries 32-47)
IRQ  0, 32    ; timer
IRQ  1, 33    ; keyboard
IRQ  2, 34
IRQ  3, 35
IRQ  4, 36
IRQ  5, 37
IRQ  6, 38
IRQ  7, 39
IRQ  8, 40
IRQ  9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

; common stub for CPU exceptions
; saves all general purpose registers then calls C handler
isr_common_stub:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp        ; pass pointer to saved registers as argument
    call isr_common_handler

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16         ; pop int_no and err_code
    iretq               ; return from interrupt

; common stub for hardware IRQs — same but calls irq handler
irq_common_stub:
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp
    call irq_common_handler

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 16
    iretq

; table of all ISR stub addresses
; idt.c reads this to populate the IDT
section .data
isr_stub_table:
    dq isr_0,  isr_1,  isr_2,  isr_3,  isr_4,  isr_5,  isr_6,  isr_7
    dq isr_8,  isr_9,  isr_10, isr_11, isr_12, isr_13, isr_14, isr_15
    dq isr_16, isr_17, isr_18, isr_19, isr_20, isr_21, isr_22, isr_23
    dq isr_24, isr_25, isr_26, isr_27, isr_28, isr_29, isr_30, isr_31
    dq irq_0,  irq_1,  irq_2,  irq_3,  irq_4,  irq_5,  irq_6,  irq_7
    dq irq_8,  irq_9,  irq_10, irq_11, irq_12, irq_13, irq_14, irq_15

section .note.GNU-stack noalloc noexec nowrite progbits