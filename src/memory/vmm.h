#pragma once
#include <stdint.h> 


#define KERNEL_VIRT_BASE 0xFFFFFFFF80000000

void vmm_init(); 
void vmm_map(uint64_t virt, uint64_t phys, int64_t flags); 
void vmm_unmap(uint64_t virt); 
uint64_t vmm_get_phys(uint64_t virt); 