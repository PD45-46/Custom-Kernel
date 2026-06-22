#pragma once
#include <stdint.h>

void     u_write   (const char *buf, uint64_t len);
void     u_yield   (void);
void     u_exit    (void);
void     u_sleep   (uint64_t ticks);
uint64_t u_read    (char *buf, uint64_t len);
uint8_t *u_map_fb  (void);
char     u_getkey  (void);
int      u_open    (const char *path);
int      u_fread   (int fd, void *buf, uint64_t len);
int      u_fseek   (int fd, int64_t off, int whence);
void     u_fclose  (int fd);
int64_t  u_fsize   (int fd);
void    *u_sbrk    (int64_t increment);
uint64_t u_gettime (void);
void     u_setpalette(const uint8_t *palette768);
void     u_set_fs_base(uint64_t base); 
