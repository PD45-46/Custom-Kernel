#include "process.h" 
#include "../memory/heap.h"
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
    proc->next = NULL; 


    /* 
    Setup the inital stack frame. 
    */

    uint64_t *sp = (uint64_t *)(stack + KERNEL_STACK_SIZE); 
    
    sp--; 
    *sp = (uint64_t)entry; 



    sp--; 
    *sp = (uint64_t)process_trampoline_fn; 


    proc->context.rsp = (uint64_t)sp; 

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
 * 
 */
// void process_trampoline_fn(void) { 
//     uint64_t entry_addr; 
//     asm volatile("pop %0" : "=r"(entry_addr)); 
//     // asm volatile("outb %0, %1" : : "a"((uint8_t)0x20), "Nd"((uint16_t)0x20));
//     pic_send_eoi(0); 
//     asm volatile("sti"); 

//     void (*entry)(void) = (void (*)(void))entry_addr; 
//     entry(); 
    
//     vga_print("[KERNEL] process returned from entry\n"); 
//     for(;;) asm volatile("hlt");
// }

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