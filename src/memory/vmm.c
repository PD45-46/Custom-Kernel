#include "vmm.h"
#include "pmm.h"
#include "../drivers/vga.h"
#include <stdint.h> 
#include <stddef.h> 
#include <string.h> 

/* TODO CLEAN UP COMMENTS... */

// uint64_t *kernel_pml4; 



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

        if(!frame) { 
            vga_print("[VMM] pmm_alloc failed in get_or_create_table\n"); 
            for(;;) asm volatile("hlt"); 
        }

        uint64_t *table = (uint64_t *)frame; 
        for(int i = 0; i < NUM_PAGE_LEVEL_ENTRIES; i++) { 
            table[i] = 0; 
        }
        *entry = (uint64_t)frame | flags | PTE_PRESENT;
        // *entry = (uint64_t)frame | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    } else { 
        if(*entry & PTE_HUGE) { 
            vga_print("[VMM] FATAL: Tried to traverse a huge page\n"); 
            for(;;) asm volatile("hlt"); 
        }
        if(flags & PTE_USER) { 
            *entry |= PTE_USER; 
        }
        if(flags & PTE_WRITABLE) { 
            *entry |= PTE_WRITABLE;
        }
        if(flags & PTE_PRESENT) { 
            *entry |= PTE_PRESENT; 
        }
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
    uint64_t cur_cr3; 
    asm volatile("mov %%cr3, %0" : "=r"(cur_cr3)); 
    uint64_t *pml4 = (uint64_t *)ENTRY_ADDR(cur_cr3); 

    uint64_t *pdpt = get_or_create_table(&pml4[PML4_INDEX(virt)], PTE_WRITABLE | PTE_USER);
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
    uint64_t cur_cr3; 
    asm volatile("mov %%cr3, %0" : "=r"(cur_cr3));
    uint64_t *pml4 = (uint64_t *)ENTRY_ADDR(cur_cr3); 

    uint64_t pml4e = pml4[PML4_INDEX(virt)]; 
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
    uint64_t cur_cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cur_cr3));
    uint64_t *pml4 = (uint64_t *)ENTRY_ADDR(cur_cr3);

    uint64_t pml4e = pml4[PML4_INDEX(virt)];
    if (!(pml4e & PTE_PRESENT)) return 0;
    uint64_t *pdpt = (uint64_t *)ENTRY_ADDR(pml4e);

    uint64_t pdpte = pdpt[PDPT_INDEX(virt)];
    if (!(pdpte & PTE_PRESENT)) return 0;
    /* check for 1GB huge page */
    if (pdpte & PTE_HUGE)
        return ENTRY_ADDR(pdpte) + (virt & 0x3FFFFFFFULL);
    uint64_t *pd = (uint64_t *)ENTRY_ADDR(pdpte);

    uint64_t pde = pd[PD_INDEX(virt)];
    if (!(pde & PTE_PRESENT)) return 0;
    /* check for 2MB huge page — this is what the bootloader uses */
    if (pde & PTE_HUGE)
        return ENTRY_ADDR(pde) + (virt & 0x1FFFFFULL);
    uint64_t *pt = (uint64_t *)ENTRY_ADDR(pde);

    uint64_t pte = pt[PT_INDEX(virt)];
    if (!(pte & PTE_PRESENT)) return 0;
    return ENTRY_ADDR(pte); 
}

/**
 * Paging is already enabled by the bootloader. Just note the 
 * current CR3 value and work off that. 
 * CR3 holds the physical address of current PML4 table. 
 */
void vmm_init(void) { 
    uint64_t cr3; 
    asm volatile("mov %%cr3, %0" : "=r"(cr3)); 
    // vga_print("Physical CR3: 0x"); vga_print_hex(cr3); vga_print("\n");
    
    // Test if the VMM can find its own code
    // uint64_t test_phys = vmm_get_phys((uint64_t)vmm_init);
    // vga_print("VMM_INIT Phys: 0x"); vga_print_hex(test_phys); vga_print("\n");
    uint64_t *pml4 = (uint64_t *)ENTRY_ADDR(cr3); 
    uint64_t *pdpt = (uint64_t *)ENTRY_ADDR(pml4[0]);
    uint64_t *pd = (uint64_t *)ENTRY_ADDR(pdpt[0]);
    
    for(int i = 0; i < 512; i++) { 
        if(!(pd[i] & PTE_PRESENT)) { 
            pd[i] = ((uint64_t)i << 21) | PTE_PRESENT | PTE_WRITABLE | PTE_HUGE; 
        }
    }
    // flush tlb 
    asm volatile("mov %%cr3, %0\n\tmov %0, %%cr3" : "=r"(cr3) :: "memory");

    vga_print("Physical CR3: 0x"); vga_print_hex(cr3); vga_print("\n");
    uint64_t test_phys = vmm_get_phys((uint64_t)vmm_init);
    vga_print("VMM_INIT Phys: 0x"); vga_print_hex(test_phys); vga_print("\n");


}


/**
 * @brief Allocates a new PML4 and maps the kernel to it's upper half. 
 *        Every user process needs a kernel mapped to it's upper half so 
 *        that when a syscall is used and the CPU stays in the same address
 *        space, the kernel code and data are accessible. 
 * 
 *        Lower half entries of the process start empty --- the process gets 
 *        no user mappings yet. 
 * 
 * @return uint64_t Physical address of the new PML4.  
 */
uint64_t vmm_create_address_space(void) { 
    uint64_t *new_pml4 = (uint64_t *)pmm_alloc(); 
    if(!new_pml4) return 0; 

    /* Get current PML4 to copy kernel mappings from */
    uint64_t cur_cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cur_cr3));
    uint64_t *cur_pml4 = (uint64_t *)ENTRY_ADDR(cur_cr3);

    // /* Zero out PML4 */
    // for(int i = 0; i < NUM_PAGE_LEVEL_ENTRIES; i++) new_pml4[i] = 0;

    // /* 
    // Copy upper half of entries (index 256 - 511) 
    // 256 corresponds to the virtual address 0xFFFF800000000000
    // These entries map the kernel --- shared across all processes. 
    // */
    // for(int i = 256; i < NUM_PAGE_LEVEL_ENTRIES; i++) { 
    //     new_pml4[i] = cur_pml4[i]; 
    // }

    for(int i = 0; i < NUM_PAGE_LEVEL_ENTRIES; i++) { // 512
        new_pml4[i] = cur_pml4[i]; 
        if(new_pml4[i] & PTE_PRESENT) { 
            new_pml4[i] |= PTE_USER; 
        }
    }
    
    return (uint64_t)new_pml4; 
}

/**
 * @brief Loads a new PML4 into CR3. This is what the scheduler calls 
 *        on every context switch when switching between processes with 
 *        different address spaces. 
 * 
 * @param pml4_phys PML4 physical address 
 */
void vmm_switch_address_space(uint64_t pml4_phys) { 
    asm volatile("mov %0, %%cr3" : : "r"(pml4_phys) : "memory"); 
}

/**
 * @brief Map a virtual address in a specific address space rather than 
 *        the current one. Temporarily switches the address space, maps 
 *        the page, then switches back.
 * 
 *        Used during process setup before the process's address space is loaded. 
 * 
 * @param pml4_phys Physical address of the new process's PML4 table. 
 * @param virt The virtual address where to process expects the data to be. 
 * @param phys Physical address of the actual RAM frame to link to virtual address. 
 * @param flags For permissions. 
 */
void vmm_map_in(uint64_t pml4_phys, uint64_t virt, uint64_t phys, uint64_t flags) { 
    uint64_t cur_cr3; 
    asm volatile("mov %%cr3, %0" : "=r"(cur_cr3)); 
    vmm_switch_address_space(pml4_phys); 
    vmm_map(virt, phys, flags);
    vmm_switch_address_space(cur_cr3);  
}

