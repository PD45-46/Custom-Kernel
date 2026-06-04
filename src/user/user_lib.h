#pragma once 
#include <stdint.h> 

void u_write(const char *buf, uint64_t len); 
void u_yield(void); 
void u_exit(void); 