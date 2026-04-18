#pragma once
#include <stdint.h> 
#include <stddef.h> 

/*
Tracks which 4KB page frames are used or free. Every time we need memory for 
a page table, a kernel stack, or process --- we ask the Physical Memory Manager 
for a frame. When done, return it. 


*/

#define PAGE_SIZE 4096

void pmm_init(void); 
void *pmm_alloc(void); // ret 4KB frame 
void pmm_free(void *frame); 
size_t pmm_free_frames(void); 