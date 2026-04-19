#pragma once
#include <stdint.h> 

/*
When a user program executes a syscall instruction, the CPU does the following: 
    - Saves RIP (return address) in RCX 
    - Saves RFLAGS into R11
    - Loads new RIP from LSTAR MSR (handler address) 
    - Switches CS to the kernel code selector
    - Note: Do not switch stack; remain on user stack 

Syscall Convention: 
    - RAX = syscall number 
    - RDI = arg 1 
    - RSI = arg 2
    - RDX = arg 3
    - R10 = arg 4 (RCX is clobbered by syscall saving RIP)
    - RAX = return value 

*/



#define SYS_WRITE 0 
#define SYS_EXIT  1
#define SYS_YIELD 2
#define SYSGETPID 3






void syscall_init(); 

int64_t syscall_dispatch(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3); 