#pragma once 
#include <stdint.h> 

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define MAX_FDS 64 

typedef struct { 
    const uint8_t *data;  
    uint64_t size; 
    uint64_t pos; 
    int in_use; 
} fd_entry_t; 

void ramdisk_init(void);
int ramdisk_open(const char *path); 
int ramdisk_read(int fd, void *buf, uint64_t len); 
int ramdisk_seek(int fd, int64_t offset, int whence); 
void ramdisk_close(int fd); 
int64_t ramdisk_size(int fd);

