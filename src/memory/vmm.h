#pragma once
#include <stdint.h> 

/*
The Virtual Memory Manager (VMM) manages the CPU's page tables --- the 
data structures that map virtual addresses to physical addresses. When 
memory is accessed, the CPU's MMU walks the page table to translate 
virtual space to a physical one. The VMM will let me create, modify, 
and destroy the mappings.  
*/

#define KERNEL_VIRT_BASE 0xFFFFFFFF80000000

void vmm_init(); 
void vmm_map(uint64_t virt, uint64_t phys, uint64_t flags); 
void vmm_unmap(uint64_t virt); 
uint64_t vmm_get_phys(uint64_t virt); 