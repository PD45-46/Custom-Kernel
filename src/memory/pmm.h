#pragma once
#include <stdint.h> 
#include <stddef.h> 

#define PAGE_SIZE 4096

void pmm_init(); 
void *pmm_alloc(); // ret 4KB frame 
void pmm_free(void *frame); 
size_t pmm_free_frames(); 