#pragma once
#include <stdint.h> 

/*
Unlike VGA, this will send information to the QEMU's terminal directly.
This means that the information will survive crashes that corrupt the VGA
and will allow for logging without touching the screen. 
Will be a main component for testing interface. 
*/

void serial_init(void); 
void serial_putchar(char c); 
void serial_print(const char *str);
void serial_print_hex(uint64_t val);
void serial_print_int(int64_t val);