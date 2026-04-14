#pragma once
#include <stdint.h> 

typedef struct __attribute__((packed)) { 
    uint16_t offset_low; 
    uint16_t segment_selector; 
    uint8_t ist; 
    uint8_t type_attr;
    uint16_t offset_mid; 
    uint32_t offset_high; 
    uint32_t zero; 
} idt_entry_t; 

typedef struct __attribute__((packed)) { 
    uint16_t limit; 
    uint64_t base; 
} idt_ptr_t; 

typedef struct { 
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t int_no, err_code;
    uint64_t rip, cs, rflags, rsp, ss;   /* pushed by CPU */
} registers_t; 

void idt_init(); 
void idt_register_handler(uint8_t n, void (*handler)(registers_t *)); 