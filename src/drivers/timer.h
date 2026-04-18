#pragma once
#include <stdint.h> 

/*
Every tick of the timer is a 'chance' to switch from one process to another. 
*/

void timer_init(uint32_t frequency);
uint64_t timer_ticks(void);  
