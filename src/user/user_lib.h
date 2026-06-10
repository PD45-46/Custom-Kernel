#pragma once 
#include <stdint.h> 

void u_write(const char *buf, uint64_t len); 
void u_yield(void); 
void u_exit(void); 
void u_sleep(uint64_t ticks); 
uint64_t u_read(char *buf, uint64_t len); 
uint8_t *u_map_fb(void); 