#include "user_lib.h"
void u_write(const char *buf, uint64_t len) { 
    asm volatile("mov $0,%%rax\nsyscall\n"
        :: "D"((uint64_t)buf), "S"((uint64_t)len)
        : "rax","rcx","rdx","r8","r9","r10","r11","memory"); 
}

void u_sleep(uint64_t ticks) { 
    asm volatile("mov $4,%%rax\nsyscall\n"
        :: "D"(ticks)
        : "rax","rcx","rdx","rsi","r8","r9","r10","r11","memory");
}

uint64_t u_read(char *buf, uint64_t len) { 
    int64_t ret;
    asm volatile("mov $5,%%rax\nsyscall\n"
        : "=a"(ret) : "D"((uint64_t)buf), "S"(len)
        : "rcx","rdx","r8","r9","r10","r11","memory");
    return ret;
}

void u_yield(void) { 
    asm volatile("mov $2,%%rax\nsyscall\n"
        ::: "rax","rcx","rdx","rsi","rdi","r8","r9","r10","r11"); 
}
void u_exit(void) { 
    asm volatile("mov $1,%%rax\nsyscall\n"
        ::: "rax","rcx","rdx","rsi","rdi","r8","r9","r10","r11");
}
uint8_t *u_map_fb(void) {
    int64_t ret;
    asm volatile("mov $6,%%rax\nsyscall\n"
        : "=a"(ret) :: "rcx","rdx","rsi","rdi","r8","r9","r10","r11","memory");
    return (uint8_t *)ret;
}
char u_getkey(void) {
    int64_t ret;
    asm volatile("mov $7,%%rax\nsyscall\n"
        : "=a"(ret) :: "rcx","rdx","rsi","rdi","r8","r9","r10","r11","memory");
    return (char)ret;
}