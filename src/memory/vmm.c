#include "vmm.h"
#include "pmm.h"
#include "../drivers/vga.h"
#include <stdint.h> 
#include <stddef.h> 
#include <string.h> 

/* TODO CLEAN UP COMMENTS... */


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

#define PTE_PRESENT (1ULL << 0)   /* page is in memory */
#define PTE_WRITABLE (1ULL << 1)   /* page can be written */
#define PTE_USER (1ULL << 2)   /* accessible from ring 3 */
#define PTE_HUGE (1ULL << 7)   /* 2MB page instead of 4KB */

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
#define PD_INDEX(addr) (((addr) >> 21) & 0x1FF)
#define PT_INDEX(addr) (((addr) >> 12) & 0x1FF)

#define NUM_PAGE_LEVEL_ENTRIES 512 

// mask to strip flags and get physical address from entry. 
#define ENTRY_ADDR(entry) ((entry) & ~0xFFFULL)

uint64_t *kernel_pml4; 

/**
 * @brief Given a page table entry, get or create the next-level 
 *        table address the table points to. 
 * 
 * @param entry Page table entry 
 * @param flags PTE_WRITABLE, PTE_USER, etc. 
 * @return uint64_t* 
 */
static uint64_t *get_or_create_table(uint64_t *entry, uint64_t flags) { 
    if(!(*entry & PTE_PRESENT)) { 
        void *frame = pmm_alloc(); 
        uint64_t *table = (uint64_t *)frame; 
        for(int i = 0; i < NUM_PAGE_LEVEL_ENTRIES; i++) { 
            table[i] = 0; 
        }
        *entry = (uint64_t)frame | flags | PTE_PRESENT; 
    }
    return (uint64_t *)ENTRY_ADDR(*entry); 
}

/**
 * @brief Creates a virtual memory to physical memory map. 
 *        Walks the 4-level page table, creating tables as needed, 
 *        and installs the final mapping at the PT level  
 * 
 * @param virt virtual address (what the program uses)
 * @param phys physical address (actual RAM location)
 * @param flags PTE_WRITABLE, PTE_USER, etc.  
 */
void vmm_map(uint64_t virt, uint64_t phys, uint64_t flags) { 
    // uint64_t pml4_phys; 
    // asm volatile("mov %%cr3, %0" : "=r"(pml4_phys)); 
    // uint64_t *pml4 = (uint64_t *)ENTRY_ADDR(pml4_phys);

    uint64_t *pdpt = get_or_create_table(&kernel_pml4[PML4_INDEX(virt)], PTE_WRITABLE | PTE_USER);
    uint64_t *pd = get_or_create_table(&pdpt[PDPT_INDEX(virt)], PTE_WRITABLE | PTE_USER); 
    uint64_t *pt = get_or_create_table(&pd[PD_INDEX(virt)], PTE_WRITABLE | PTE_USER);
    pt[PT_INDEX(virt)] = phys | flags | PTE_PRESENT; 

    // flush TLB (Transition Lookaside Buffer) for this address so CPU uses new mapping 
    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

/**
 * @brief Removes the mapping for the virtual address and 
 *        flushes TLB (Transition Lookaside Buffer). 
 * 
 * @param virt virtual address
 */
void vmm_unmap(uint64_t virt) { 
    // uint64_t pml4_phys; 
    // asm volatile("mov %%cr3, %0" : "=r"(pml4_phys));
    // uint64_t *pml4 = (uint64_t *)ENTRY_ADDR(pml4_phys); 

    uint64_t pml4e = kernel_pml4[PML4_INDEX(virt)]; 
    if(!(pml4e & PTE_PRESENT)) return; 
    uint64_t *pdpt = (uint64_t *)ENTRY_ADDR(pml4e); 

    uint64_t pdpte = pdpt[PDPT_INDEX(virt)];
    if (!(pdpte & PTE_PRESENT)) return;
    uint64_t *pd = (uint64_t *)ENTRY_ADDR(pdpte);

    uint64_t pde = pd[PD_INDEX(virt)];
    if (!(pde & PTE_PRESENT)) return;
    uint64_t *pt = (uint64_t *)ENTRY_ADDR(pde);

    pt[PT_INDEX(virt)] = 0; 
    asm volatile("invlpg (%0)" : : "r"(virt) : "memory");
}

/**
 * @brief Gets physical address associated with virtual address. 
 * 
 * @param virt virtual address
 * @return uint64_t physical address 
 */
uint64_t vmm_get_phys(uint64_t virt) { 
    // uint64_t pml4_phys;
    // asm volatile("mov %%cr3, %0" : "=r"(pml4_phys));
    // uint64_t *pml4 = (uint64_t *)ENTRY_ADDR(pml4_phys);

    uint64_t pml4e = kernel_pml4[PML4_INDEX(virt)]; 
    if(!(pml4e & PTE_PRESENT)) return 0; 
    uint64_t *pdpt = (uint64_t *)ENTRY_ADDR(pml4e); 

    uint64_t pdpte = pdpt[PDPT_INDEX(virt)]; 
    if(!(pdpte & PTE_PRESENT)) return 0; 
    uint64_t *pd = (uint64_t *)ENTRY_ADDR(pdpte); 

    uint64_t pde = pd[PD_INDEX(virt)]; 
    if(!(pde & PTE_PRESENT)) return 0; 
    uint64_t *pt = (uint64_t *)ENTRY_ADDR(pde);
    
    return ENTRY_ADDR(pt[PT_INDEX(virt)]); 
}

/**
 * Paging is already enabled by the bootloader. Just note the 
 * current CR3 value and work off that. 
 * CR3 holds the physical address of current PML4 table. 
 */
void vmm_init() { 
    uint64_t cr3; 
    asm volatile("mov %cr3, %0" : "=r"(cr3)); 

    kernel_pml4 = (uint64_t *)ENTRY_ADDR(cr3); 
}