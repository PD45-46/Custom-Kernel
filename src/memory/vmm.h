#pragma once
#include "pmm.h"
#include <stdint.h> 

/*
The Virtual Memory Manager (VMM) manages the CPU's page tables --- the 
data structures that map virtual addresses to physical addresses. When 
memory is accessed, the CPU's MMU walks the page table to translate 
virtual space to a physical one. The VMM will let me create, modify, 
and destroy the mappings.  
*/

#define KERNEL_VIRT_BASE 0xFFFFFFFF80000000

/* 
Page Table Bitmap 

63                              12 11            0
+--------------------------------+---------------+
|   Physical Address (bits 51-12) |   Flags       |
+--------------------------------+---------------+

Bit(s)   Meaning
----------------------------------------
0        Present (P)
1        Writable (R/W)
2        User (U/S)
3        Write-through
4        Cache disable                                      -- Unhandled!
5        Accessed                                           -- Unhandled!
6        Dirty (only in PT level)                           -- Unhandled!
7        Huge page (in PD/PT levels)
8        Global
9–11     Available for OS (I can use these!!!!)
12–51    Physical address (aligned to 4KB)
52–62    Available / reserved (depends on CPU)
63       NX (No Execute) (if supported)                     -- Unhandled!
*/

/* 
Page table flags. 
The flag bits remain in the bottom 12 bits of each 
page table entry. 
*/

#define PTE_PRESENT  (1ULL << 0)   /* page is in memory */
#define PTE_WRITABLE (1ULL << 1)   /* page can be written */
#define PTE_USER     (1ULL << 2)   /* accessible from ring 3 */
#define PTE_HUGE     (1ULL << 7)   /* 2MB page instead of 4KB */

/*
x86_64 uses 4-level paging: PML4 -> PDPT -> PD -> PT 
Each page level is a page (KB) containing 512 8-byte entries. 
A virtual address breaks down as: 
    bits 49-39: PML4 (Page Map Level 4)             index  
    bits 38-30: PDPT (Page Directory Pointer Table) index
    bits 29-21: PD   (Page Directory)               index
    bits 20-12: PT   (Page Table)                   index
    bits 11-0: page offset
*/

#define PML4_INDEX(addr) (((addr) >> 39) & 0x1FF)
#define PDPT_INDEX(addr) (((addr) >> 30) & 0x1FF)
#define PD_INDEX(addr)   (((addr) >> 21) & 0x1FF)
#define PT_INDEX(addr)   (((addr) >> 12) & 0x1FF)

#define NUM_PAGE_LEVEL_ENTRIES (PAGE_SIZE/8) 

// mask to strip flags and get physical address from entry. 
#define ENTRY_ADDR(entry) ((entry) & ~0xFFFULL)

void vmm_init(void); 
void vmm_map(uint64_t virt, uint64_t phys, uint64_t flags); 
void vmm_unmap(uint64_t virt); 
uint64_t vmm_get_phys(uint64_t virt); 

uint64_t vmm_create_address_space(void); 
void vmm_switch_address_space(uint64_t pml4_phys); 
void vmm_map_in(uint64_t pml4_phys, uint64_t virt, uint64_t phys, uint64_t flags); 