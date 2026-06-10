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

void u_sleep(uint64_t ticks) { 
    asm volatile(
        "mov $4, %%rax\n syscall\n"
        : 
        : "D"(ticks)
        : "rax", "rcx", "r11", "memory"
    );
}

uint64_t u_read(char *buf, uint64_t len) { 
    int64_t ret;
    asm volatile(
        "mov $5, %%rax\n syscall\n"
        : "=a"(ret)
        : "D"((uint64_t)buf), "S"(len)
        : "rcx", "r11", "memory"
    );
    return ret;
}

uint8_t *u_map_fb(void) { 
    int64_t ret; 
    asm volatile(
        "mov $6, %%rax\n syscall\n"
        : "=a"(ret) 
        :
        : "rcx", "r11", "memory"
    );
    return (uint8_t *)ret; 
}