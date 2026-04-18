#include "gdt.h"
#include <stdint.h> 

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
extern void gdt_flush(uintptr_t gdt_ptr); 

void gdt_init(void) { 
    gdt_ptr.limit = (sizeof(gdt_entry_t) * 5) - 1; 
    gdt_ptr.base = (uintptr_t)&gdt; 

    gdt_set_entry(0, 0, 0, 0x00, 0x00); 
    gdt_set_entry(1, 0, 0xFFFFFFFF, 0x9A, 0xA0); 
    gdt_set_entry(2, 0, 0xFFFFFFFF, 0x92, 0xC0); 
    gdt_set_entry(3, 0, 0xFFFFFFFF, 0xFA, 0xA0); 
    gdt_set_entry(4, 0, 0xFFFFFFFF, 0xF2, 0xC0);
    
    gdt_flush((uintptr_t)&gdt_ptr); 

}



