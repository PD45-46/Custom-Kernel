#include "user_lib.h"

void u_write(const char *buf, uint64_t len) { 
    asm volatile(
        "mov $0, %%rax\n"
        "syscall\n"
        : 
        : "D"((uint64_t)buf), "S"((uint64_t)len)
        : "rax","rcx","r11","memory"
    ); 
}

void u_yield(void) { 
    /*
    Implement slow factor: 
    int i = 0; 
    while(i-- > 0) {} 
    
    
    */
    asm volatile(
        "mov $2, %%rax\n"
        "syscall\n"
        : : : "rax","rcx","rdx","rsi","rdi","r8","r9","r10","r11"
    ); 
}

void u_exit(void) { 
    asm volatile(
        "mov $1, %%rax\n"
        "syscall\n"
        : : : "rax","rcx","r11"
    );
}