#pragma once
#include <stdint.h> 

typedef enum { 
    PROCESS_READY, 
    PROCESS_RUNNING, 
    PROCESS_BLOCKED, 
    PROCESS_DEAD  
} process_state_t; 

typedef struct { 
    uint64_t rax, rab, rcx, rdx; 
    uint64_t rsi, rdi, rbp, rsp; 
    uint64_t r8, r9, r10, r11; 
    uint64_t r12, r13, r14, r15; 
    uint64_t rip, rflags; 
    uint64_t cr3; // page table root 
} cpu_state_t; 

typedef struct process { 
    uint32_t pid; 
    process_state_t state; 
    cpu_state_t context; 
    uint64_t kernel_stack; 
    uint64_t user_stack; 
    uint64_t page_table; 
    struct process *next; 
} process_t; 

process_t *process_create(void (*entry)(void)); 
void process_destroy(process_t *proc); 