#pragma once
#include <stdint.h> 

typedef enum { 
    PROCESS_READY, 
    PROCESS_RUNNING, 
    PROCESS_BLOCKED, 
    PROCESS_DEAD  
} process_state_t; 

typedef struct { 
    uint64_t rsp;
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
    uint64_t rip, rflags;
    uint64_t cr3;
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

#define USER_STACK_VIRT  0x8000002000ULL  /* top of user stack VA */
#define USER_STACK_PAGES 4                /* 16KB user stack      */
#define USER_CODE_VIRT   0x8000000000ULL  /* where user code lives */

process_t *process_create(void (*entry)(void)); 
process_t *process_create_user(void (*entry)(void)); 
void process_trampoline_fn(void); 
void process_destroy(process_t *proc); 