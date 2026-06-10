#include "process.h" 
#include "../memory/heap.h"
#include "../memory/vmm.h"
#include "../drivers/vga.h"
#include "../drivers/pic.h"
#include <stdint.h> 
#include <stddef.h>

/*
Every process gets a 16KB kernel stack. 
The stack is used when the process is executing kernel 
code --- interrupts handlers, syscalls, etc. 
*/
#define KERNEL_STACK_SIZE 16384

static uint32_t next_pid = 1; 

extern void process_trampoline_fn(void); 
extern void process_user_trampoline_fn(void);

_Static_assert(offsetof(process_t, page_table) == 176,
               "update PROCESS_PAGE_TABLE in context.asm");
_Static_assert(offsetof(process_t, context.rsp) == 8, "RSP offset mismatch");
_Static_assert(sizeof(process_t) > 176, "Process struct is too small");

/**
 * @brief Allocates and initialises a new process. 
 * 
 * @param entry Function pointer --- where this process starts executing
 * @return process_t* Pointer to the new process_t or NULL on failure 
 * @note Build the fake inital stack such that it grows downwards. When 
 *       context switch does 'ret' we will enter the trampoline. 
 */
process_t *process_create(void (entry)(void)) { 
    process_t *proc = kcalloc(1, sizeof(process_t)); 
    if(!proc) return NULL; 
    
    uint8_t *stack = (uint8_t *)kmalloc(KERNEL_STACK_SIZE); 
    if(!stack) { 
        kfree(proc); 
        return NULL; 
    }

    proc->pid = next_pid++; 
    proc->state = PROCESS_READY; 
    proc->kernel_stack = (uint64_t)stack + KERNEL_STACK_SIZE; 
    proc->page_table = 0; 
    proc->next = NULL; 


    /* 
    Setup the inital stack frame. 
    */

    uint64_t *sp = (uint64_t *)(stack + KERNEL_STACK_SIZE); 
    
    sp--; *sp = (uint64_t)entry; 

    sp--; *sp = (uint64_t)process_trampoline_fn; 

    proc->context.rsp = (uint64_t)sp; 

    return proc; 
}


/**
 * @brief 
 * 
 * @param entry 
 * @return process_t* 
 */
process_t *process_create_user(void (entry)(void)) { 
    process_t *proc = kcalloc(1, sizeof(process_t)); 
    if(!proc) return NULL; 

    /* kernel stack --- used when handling syscalls/interrupts */
    uint8_t *kstack = kmalloc(KERNEL_STACK_SIZE); 
    if(!kstack) {
        kfree(proc); 
        return NULL;  
    }

    proc->pid = next_pid++; 
    proc->state = PROCESS_READY; 
    proc->kernel_stack = (uint64_t)kstack + KERNEL_STACK_SIZE; 
    proc->next = NULL; 

    /* create private address space */
    proc->page_table = vmm_create_address_space(); 
    if(!proc->page_table) { 
        kfree(kstack);
        kfree(proc); 
        return NULL;  
    }

    /* 
    Map user stack pages into the new address space. Stack 
    grows downwards so map pages below USER_STACK_VIRT 
    VMM_FLAG_USER makes them accessible from ring 3. 
    */
    for(int i = 0; i < USER_STACK_PAGES; i++) { 
        void *frame = pmm_alloc(); 
        if(!frame) return NULL; /* TODO Add cleanup */
        uint64_t virt = USER_STACK_VIRT - ((USER_STACK_PAGES - i) * PAGE_SIZE); 
        vmm_map_in(proc->page_table, virt, (uint64_t)frame, PTE_PRESENT | PTE_WRITABLE | PTE_USER); 
    }
    proc->user_stack = USER_STACK_VIRT; 

    /* 
    Map the entry function's physical page as user accessible. 
    Find the physical address of the entry function and map it 
    at USER_CODE_VIRT in the new address space. 

    For now, entry is a kernel function --- map it directly. 
    Later, when ELF files are loaded, this will be different. 
    */
    uint64_t entry_phys = vmm_get_phys(ENTRY_ADDR((uint64_t)entry)); 
    // uint64_t entry_phys = (uint64_t)entry & ~0xFFFULL;
    // vmm_map_in(proc->page_table, USER_CODE_VIRT, entry_phys, PTE_PRESENT | PTE_USER); 
    // vmm_map_in(proc->page_table, USER_CODE_VIRT + 0x1000, entry_phys + 0x1000, PTE_PRESENT | PTE_USER);
    
    for(int i = 0; i < 8; i++) { 
        vmm_map_in(proc->page_table,
               USER_CODE_VIRT + i * 0x1000,
               entry_phys + i * 0x1000,
               PTE_PRESENT | PTE_USER);
    }

    
    uint64_t entry_virt = USER_CODE_VIRT + ((uint64_t)entry & 0xFFFULL); 
    uint64_t entry_page_virt = (uint64_t)entry & ~0xFFFULL;

    vga_print("entry fn addr:  "); vga_print_hex((uint64_t)entry);      vga_print("\n");
    vga_print("entry page virt:"); vga_print_hex(entry_page_virt);       vga_print("\n");
    vga_print("entry_phys:     "); vga_print_hex(entry_phys);            vga_print("\n");
    vga_print("entry_virt:     "); vga_print_hex(entry_virt);            vga_print("\n");
    vga_print("user page_table:"); vga_print_hex(proc->page_table);      vga_print("\n");

    if (entry_phys == 0) {
        vga_print("ERROR: vmm_get_phys returned 0 — kernel fn not mapped\n");
        for(;;) asm volatile("hlt");
    }

    /* verify the mapping took effect */
    uint64_t verify_cur, verify_phys;
    asm volatile("mov %%cr3, %0" : "=r"(verify_cur));
    vmm_switch_address_space(proc->page_table);
    verify_phys = vmm_get_phys(USER_CODE_VIRT);
    vmm_switch_address_space(verify_cur);

    vga_print("verify mapping: "); vga_print_hex(verify_phys); vga_print("\n");
    if (verify_phys == 0) {
        vga_print("ERROR: mapping not installed in user PML4\n");
        for(;;) asm volatile("hlt");
    }
    
    /*
    Setup inital kernel stack with retq frame. 
    When context_switch ret's into process_trampoline_fn, the
    trampoline builds this retq frame and executes it to drop 
    into ring 3. 

    Store the user entry VA and user RSP so the trampoline can 
    build the frame. 
    */
    uint64_t *sp = (uint64_t *)(kstack + KERNEL_STACK_SIZE); 
    // sp--; *sp = 0; // padding 
    sp--; *sp = proc->user_stack;
    sp--; *sp = entry_virt; 
    sp--; *sp = (uint64_t)process_user_trampoline_fn; 

    proc->context.rsp = (uint64_t)sp; 
    vga_print("proc->context.rsp="); 
    vga_print_hex(proc->context.rsp); 
    vga_print("\n"); 
    return proc; 
}


/**
 * @brief Landing pad for new processes. After performing a context switch, 
 *        the process stored in entry_addr will be invoked and run until 
 *        either the timer runs out or the process completes. The CPU will 
 *        then be ready for the next interrupt. The trampoline simply redirects 
 *        to the entry address. 
 * 
 * @note The trampoline is only ever performed once --- when the process runs
 *       for the first time. 
 *       TRAMPOLINE IS NOW WRITTEN IN process_asm.asm 
 * 
 */


/**
 * @brief Frees the memory associated with the process, essentially 
 *        destroying it. 
 * 
 * @param proc Process to destroy 
 */
void process_destroy(process_t *proc) { 
    if(!proc) return; 
    uint8_t *stack_base = (uint8_t *)(proc->kernel_stack - KERNEL_STACK_SIZE); 
    kfree(stack_base);
    kfree(proc);  
}