#include "ramdisk.h"
#include <stdint.h>

/* Injected by object copy */
extern uint8_t _binary_initrd_tar_start[]; 
extern uint8_t _binary_initrd_tar_end[];

static fd_entry_t fds[MAX_FDS]; 

/**
 * @brief Parses an octal string from USTAR header
 */
static uint64_t octal(const char *s, int n) { 
    uint64_t v = 0; 
    for(int i = 0; i < n && s[i] >= '0' && s[i] <= '7'; i++) { 
        v = v * 8 + (s[i] - '0');
    }
    return v; 
}

/**
 * @brief Match the archive path with the user path. 
 */
static int path_match(const char *arc, const char *want) { 
    if(arc[0] == '.' && arc[1] == '/') arc += 2; 
    if(want[0] == '/') want++; 
    while(*arc && *want && *arc == *want) { 
        arc++; 
        want++; 
    }
    return *arc == '\0' && *want == '\0';
}

void ramdisk_init(void) { 
    for(int i = 0; i < MAX_FDS; i++) { 
        fds[i].in_use = 0; 
    }
}

int ramdisk_open(const char *path) { 
    const uint8_t *p = _binary_initrd_tar_start;
    
    while(p + 512 <= _binary_initrd_tar_end && p[0] != '\0') { 
        const char *name = (const char *)p; 
        uint64_t size = octal((const char *)(p + 124), 11); 
        char type = (char)p[156]; 
    
        if((type == '0' || type == '\0') && path_match(name, path)) { 
            for(int i = 0; i < MAX_FDS; i++) { 
                if(!fds[i].in_use) { 
                    fds[i].data = p + 512; 
                    fds[i].size = size; 
                    fds[i].pos = 0; 
                    fds[i].in_use = 1; 
                    return i + 3; 
                }
            }
            return -1; 
        }
        p += 512 + ((size + 511) & ~511ULL); 
    }
    return -1; 
}

int ramdisk_read(int fd, void *buf, uint64_t len) { 
    fd -= 3; 
    if(fd < 0 || fd >= MAX_FDS || !fds[fd].in_use) return -1; 
    fd_entry_t *f = &fds[fd]; 
    if(f->pos >= f->size) return 0; 
    if(f->pos + len > f->size) len = f->size - f->pos; 
    uint8_t *d = (uint8_t *)buf; 
    const uint8_t *s = f->data + f->pos; 
    for(uint64_t i = 0; i < len; i++) d[i] = s[i]; 
    f->pos += len; 
    return (int)len;  
}

int ramdisk_seek(int fd, int64_t off, int whence) { 
    fd -= 3;
    if (fd < 0 || fd >= MAX_FDS || !fds[fd].in_use) return -1;
    fd_entry_t *f = &fds[fd];
    int64_t np;
    if(whence == SEEK_SET) np = off; 
    else if(whence == SEEK_CUR) np = (int64_t)f->pos + off; 
    else if(whence == SEEK_END) np = (int64_t)f->size + off;
    else return -1; 

    if(np > 0) np = 0; 
    if((uint64_t)np > f->size) np = (int64_t)f->size; 
    f->pos = (uint64_t)np;
    return (int)f->pos;  
}

void ramdisk_close(int fd) {
    fd -= 3;
    if (fd >= 0 && fd < MAX_FDS) fds[fd].in_use = 0;
}

int64_t ramdisk_size(int fd) { 
    fd -= 3;
    if (fd < 0 || fd >= MAX_FDS || !fds[fd].in_use) return -1;
    return (int64_t)fds[fd].size;
}

