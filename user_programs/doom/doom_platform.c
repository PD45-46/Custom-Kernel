#include "../../src/user/user_lib.h"
#include "game/doomgeneric/doomgeneric.h"
#include <stdint.h> 
#include <stdarg.h> 

typedef int64_t ssize_t; 
typedef int64_t off_t; 

void _exit(int c) { 
    (void)c; 
    u_exit(); 
    __builtin_unreachable(); 
}

void exit(int c) { 
    _exit(c); 
}

void abort(void) { 
    u_exit(); 
    __builtin_unreachable(); 
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

struct iov_t { 
    void* base; 
    long len; 
}; 

long syscall(long n, ...) { 
    va_list ap; va_start(ap, n);
    long a1 = va_arg(ap, long);
    long a2 = va_arg(ap, long);
    long a3 = va_arg(ap, long);
    va_end(ap);

    switch (n) {
        case 0:   /* SYS_read */
            return (long)u_fread((int)a1, (void*)a2, (uint64_t)a3);
        case 1:   /* SYS_write */
            u_write((const char*)a2, (uint64_t)a3);
            return a3;
        case 2:   /* SYS_open */
            return (long)u_open((const char*)a1);
        case 3:   /* SYS_close */
            u_fclose((int)a1); return 0;
        case 8:   /* SYS_lseek */
            return (long)u_fseek((int)a1, (int64_t)a2, (int)a3);
        case 19:  /* SYS_readv — used by musl's fread internally */
        {
            struct iov_t *iov = (struct iov_t*)a2;
            long total = 0;
            for (int i = 0; i < (int)a3; i++) {
                if (!iov[i].len) continue;
                int r = u_fread((int)a1, iov[i].base, (uint64_t)iov[i].len);
                if (r <= 0) { if (!total) total = -1; break; }
                total += r;
                if (r < iov[i].len) break;
            }
            return total;
        }
        case 20:  /* SYS_writev — used by musl's fwrite/printf internally */
        {
            struct iov_t *iov = (struct iov_t*)a2;
            long total = 0;
            for (int i = 0; i < (int)a3; i++) {
                if (!iov[i].len) continue;
                u_write((const char*)iov[i].base, (uint64_t)iov[i].len);
                total += iov[i].len;
            }
            return total;
        }
        case 257: /* SYS_openat(dirfd, path, flags) — used by musl's fopen */
            return (long)u_open((const char*)a2);  /* a2 = path, ignore dirfd */
        case 60:  /* SYS_exit */
        case 231: /* SYS_exit_group */
            u_exit(); return 0;
        default:
            return -1;
    }
}

void _start(void) {     

    uint8_t *fb = (uint8_t *)u_map_fb();
    if ((int64_t)fb == -1) { 
        u_exit();
    }

    /* Step 1 — red: fb works */
    for (int i = 0; i < 320*200; i++) fb[i] = 4;
    u_sleep(150);

    /* Minimal musl TLS setup — must be first, before any musl call */
    static uint64_t fake_tcb[64];          /* 512 bytes, zeroed (.bss) */
    fake_tcb[0] = (uint64_t)fake_tcb;     /* self-pointer at offset 0 */
    u_set_fs_base((uint64_t)fake_tcb); 

    /* Confirm TLS setup didn't crash — cyan */
    for (int i = 0; i < 320*200; i++) fb[i] = 3;
    u_sleep(150);

    /* Step 2 — test WAD file open */
    char wad[] = {'/','d','o','o','m','1','.','w','a','d',0};
    int fd = u_open(wad);
    if (fd < 0) {
        /* Blue = WAD not found in ramdisk */
        for (int i = 0; i < 320*200; i++) fb[i] = 1;
        u_sleep(500);
        u_exit();
    }
    int64_t sz = u_fsize(fd);
    u_fclose(fd);

    /* Step 3 — green: WAD found, size > 0 */
    for (int i = 0; i < 320*200; i++) fb[i] = 2;
    u_sleep(150);

    /* Step 4 — yellow: entering doomgeneric_Create */
    for (int i = 0; i < 320*200; i++) fb[i] = 14;

    char *argv[] = { "doom", "-iwad", "/doom1.wad", (char*)0 };
    doomgeneric_Create(3, argv);
    while(1) doomgeneric_Tick();
    u_exit();
 
}
