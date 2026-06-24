#include "../../src/user/user_lib.h"
#include "game/doomgeneric/doomgeneric.h"
#include <stdint.h> 
#include <stdarg.h> 

typedef int64_t ssize_t; 
typedef int64_t off_t; 

void _exit(int c) { 
    (void)c;
    asm volatile("mov $231,%%rax\nxor %%rdi,%%rdi\nsyscall\n"
        ::: "rax","rcx","rdx","rsi","rdi","r8","r9","r10","r11","memory");
    __builtin_unreachable();
}

void exit(int c) { 
    _exit(c); 
}

void abort(void) { 
    _exit(1);
}

ssize_t write(int fd, const void *buf, size_t n) {
    (void)fd;
    u_write((const char *)buf, (uint64_t)n);
    return (ssize_t)n;
}
ssize_t read(int fd, void *buf, size_t n) {
    return (ssize_t)u_fread(fd, buf, (uint64_t)n);
}
int open(const char *path, int flags, ...) {
    (void)flags; return u_open(path);
}
off_t lseek(int fd, off_t off, int whence) {
    return (off_t)u_fseek(fd, (int64_t)off, whence);
}
int close(int fd) { u_fclose(fd); return 0; }

void _start(void) {     

    uint8_t *fb = (uint8_t *)u_map_fb();          /* custom #6 → map_fb ✓ */
    if ((int64_t)fb == -1) { _exit(1); }

    for (int i = 0; i < 320*200; i++) fb[i] = 4; /* red */
    u_sleep(150);                                  /* custom #4 → sleep ✓ */

    static uint64_t fake_tcb[64];
    fake_tcb[0] = (uint64_t)fake_tcb;
    u_set_fs_base((uint64_t)fake_tcb);             /* custom #16 → set_fs_base ✓ */

    for (int i = 0; i < 320*200; i++) fb[i] = 3; /* cyan: TLS ok */
    u_sleep(150);

    for (int i = 0; i < 320*200; i++) fb[i] = 14;/* yellow: entering doom */

    char *argv[] = { "doom", "-iwad", "/doom1.wad", (char*)0 };
    doomgeneric_Create(3, argv);
    while(1) doomgeneric_Tick();
    _exit(0);
 
}


