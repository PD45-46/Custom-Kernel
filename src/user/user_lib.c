#include "user_lib.h"
#include <stdint.h> 
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
    asm volatile("mov $177,%%rax\nsyscall\n"
        : "=a"(ret) :: "rcx","rdx","rsi","rdi","r8","r9","r10","r11","memory");
    return (char)ret;
}
int u_open(const char *path) {
    int64_t r;
    asm volatile("mov $8,%%rax\nsyscall\n"
        :"=a"(r):"D"((uint64_t)path)
        :"rcx","rdx","rsi","r8","r9","r10","r11","memory");
    return (int)r;
}
int u_fread(int fd, void *buf, uint64_t len) {
    int64_t r;
    asm volatile("mov $9,%%rax\nsyscall\n"
        :"=a"(r):"D"((uint64_t)fd),"S"((uint64_t)buf),"d"(len)
        :"rcx","r8","r9","r10","r11","memory");
    return (int)r;
}
int u_fseek(int fd, int64_t off, int whence) {
    int64_t r;
    asm volatile("mov $10,%%rax\nsyscall\n"
        :"=a"(r):"D"((uint64_t)fd),"S"((uint64_t)off),"d"((uint64_t)whence)
        :"rcx","r8","r9","r10","r11","memory");
    return (int)r;
}
void u_fclose(int fd) {
    asm volatile("mov $11,%%rax\nsyscall\n"
        ::"D"((uint64_t)fd)
        :"rax","rcx","rdx","rsi","r8","r9","r10","r11","memory");
}
int64_t u_fsize(int fd) {
    int64_t r;
    asm volatile("mov $12,%%rax\nsyscall\n"
        :"=a"(r):"D"((uint64_t)fd)
        :"rcx","rdx","rsi","r8","r9","r10","r11","memory");
    return r;
}
void *u_sbrk(int64_t increment) {
    int64_t r;
    asm volatile("mov $13,%%rax\nsyscall\n"
        : "=a"(r) : "D"((uint64_t)increment)
        : "rcx","rdx","rsi","r8","r9","r10","r11","memory");
    return (void *)r;
}
uint64_t u_gettime(void) { 
    int64_t r; 
    asm volatile("mov $14,%%rax\nsyscall\n"
        : "=a"(r) :: "rcx","rdx","rsi","rdi","r8","r9","r10","r11","memory");
    return (uint64_t)r;
}
void u_setpalette(const uint8_t *palette768) {
    asm volatile("mov $15,%%rax\nsyscall\n"
        :: "D"((uint64_t)palette768)
        : "rax","rcx","rdx","rsi","r8","r9","r10","r11","memory");
}
void u_set_fs_base(uint64_t base) { 
    asm volatile(
        "mov $158, %%rax\n"   /* arch_prctl */
        "mov $0x1002, %%rdi\n" /* ARCH_SET_FS */
        "mov %0, %%rsi\n"
        "syscall\n"
        :: "r"(base)
        : "rax","rcx","rdx","rdi","rsi","r8","r9","r10","r11","memory");
} 