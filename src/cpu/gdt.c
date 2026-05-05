#include "gdt.h"
#include <stdint.h> 

void gdt_init(void); 
void syscall_init(void); 



/*
5 entries: 
0 - null descriptor (always 0)
1 - kernel code segment (ring 0, executable)
2 - kernel data segment (ring 0, writable)
3 - user code segment (ring 3, executable)
4 - user data segment (ring 3, writable)
*/
static gdt_entry_t gdt[5]; 
static gdt_ptr_t gdt_ptr; 

/** 
 * @brief Fills one GDT slot. 
 * @param i which of the gdt's to select 
 * @param base
 * @param limit
 * @param access 
 * @param gran 
 * 
 * @note Most of the fields are ignored in modern systems, but they still
 *       need to be set correctly as it is used to initiate protected mode,
 *       define memory segments, and establish protection rings. We will 
 *       use paging to handle memory translation. 
 */
static void gdt_set_entry(
    int i, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) 
{ 
    gdt[i].base_low = (base & 0xFFFF); 
    gdt[i].base_mid = (base >> 16) & 0xFF; 
    gdt[i].base_high = (base >> 24) & 0xFF;
    
    gdt[i].limit_low = (limit & 0xFFFF); 
    gdt[i].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0); 

    gdt[i].access = access; 
}


/** 
 * Defined in gdt_flush.asm 
 * Loads the GDT pointer with lgdt and does a far jump to reload the segment 
 * registers with the new descriptors. The far jump can't be done in C. 
 */
extern void gdt_flush(uint64_t gdt_ptr); 

void gdt_init(void) { 
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 6) - 1; 
    gdt_ptr.base = (uint64_t)&gdt; 

    gdt_set_entry(0, 0, 0, 0x00, 0x00); 
    gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xA0); 
    gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xC0); 
    gdt_set_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xA0); 
    gdt_set_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xC0);
    
    gdt_flush((uint64_t)&gdt_ptr); 
    syscall_init(); 

}


/**
 * @brief Write a value to a Model Specific Register. 
 *        RCX = MSR Addr 
 *        RAX = Low bits 
 *        RDX = High bits 
 * @param msr 
 * @param val 
 */
static void wrmsr(uint32_t msr, uint64_t val) { 
    uint32_t low = val & 0xFFFFFFFF;
    uint32_t high = val >> 32; 
    asm volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high)); 
}


/**
 * @brief Programs the CPU to be able to handle syscall instructions. 
 *        
 * STAR LAYOUT: 
 *  63:48 = user mode CS selector     (sysret load this, 3 for ring 3)
 *  47:32 = kernel mode CS selector   (syscall loads this for ring 0)
 * 
 * GDT: 
 *  0x80 = kernel code (ring 0)
 *  0x10 = kernel data (ring 0)
 *  0x18 = user code   (ring 3)
 *  0x20 = user data   (ring 3)
 * 
 * STAR[47:32] = 0x08 
 * STAR[63:48] = 0x18 
 * 
 * SFMASK = 0x200 clears the interrupt flag on syscall entry 
 *          so the handler can be entered with interrupts 
 *          disabled. They will be re-enabled explicitly inside 
 *          the handler after switching stacks.  
 * 
 */
void syscall_init(void) { 
    extern void syscall_entry(void); 
    wrmsr(MSR_LSTAR, (uint64_t)syscall_entry); 
    
    /* As explained in the STAR LAYOUT */
    wrmsr(MSR_STAR, ((uint64_t)0x18 << 48) | ((uint64_t)0x08 << 32)); 

    wrmsr(MSR_SFMASK, 0x200); 

    /* 
    Enable syscall instruction by setting the SCE bit in the 
    EFER (Extended Feature Enable Register) MSR. Without this, 
    executing a syscall in user mode raises undefined instruction #UD
    */

    uint32_t low, high; 
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(MSR_EFER)); 
    low |= EFER_SCE; 
    asm volatile("wrmsr" : : "c"(MSR_EFER), "a"(low), "d"(high)); 
}




