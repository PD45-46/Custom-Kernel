#include "scheduler.h"
#include "process.h"
#include "../drivers/vga.h"
#include "../drivers/timer.h"
#include "../cpu/gdt.h"
#include <stdint.h> 
#include <stddef.h> 


/*
Circular linked list of processes. Current is always the process that 
is running currently and idle is the next process to run in the list. 
On each timer tick, context switch from current to next process. 
*/
static process_t *current = NULL; 
static process_t *idle = NULL; 

static process_t boot_context; 

extern void context_switch(process_t *current, process_t *next); 

/**
 * @brief Runs when no other process is ready. Just 
 *         halts the CPU until next interrupt. 
 * 
 */
static void idle_fn(void) { 
    for(;;) asm volatile("sti\n\thlt"); 
}


/**
 * @brief 
 * 
 */
void scheduler_init(void) { 
    idle = process_create(idle_fn); 
    if(!idle) { 
        vga_print("[SCHEDULER] failed to create idle process\n"); 
        for(;;) asm volatile("hlt"); 
    }
    idle->state = PROCESS_RUNNING; 
    idle->next = idle; 
    current = idle; 
}


/**
 * @brief 
 * 
 * @param proc 
 */
void scheduler_add(process_t *proc) { 
    if(!proc) { 
        vga_print("[SCHEDULER] failed to add process\n"); 
        return; 
    }

    proc->state = PROCESS_READY; 
    process_t *tail = idle; 
    while(tail->next != idle) { 
        tail = tail->next; 
    }

    proc->next = idle; 
    tail->next = proc; 

    vga_print("[SCHEDULER] ADDED PROCESS\n"); 
}


process_t *scheduler_current(void) { 
    return current; 
}


/**
 * @brief Picks the next process in the circular list and switches to it.  
 * 
 */
void scheduler_tick(void) { 
    if(!current || !current->next) return; 

    uint64_t now = timer_ticks(); 
    process_t *p = idle->next; 
    while(p != idle) { 
        if(p->state == PROCESS_BLOCKED && 
           p->wait_reason == WAIT_SLEEP && 
           now >= p->wake_tick) { 

            p->state = PROCESS_READY; 
            p->wait_reason = WAIT_NONE; 
        }
        p = p->next; 
    }


    process_t *curr = current; 
    process_t *next = current->next; 

    while((next->state == PROCESS_DEAD || next->state == PROCESS_BLOCKED) && next != curr) { 
        next = next->next; 
    }

    if(next == curr) return; 
    if(curr->state == PROCESS_RUNNING) { 
        curr->state = PROCESS_READY; 
    }
    next->state = PROCESS_RUNNING; 
    current = next; 

    /* 
    If next is user process (has its own page table), 
    update TSS.RSP0 to hardware interrupts from ring 3 
    switch to the correct kernel stack.  
    */
    if(next->page_table) { 
            tss_set_kernel_stack(next->kernel_stack); 
    }

    context_switch(curr, next); 
}

void scheduler_wake_key_waiter(void) { 
    process_t *p = idle->next; 
    while(p != idle) { 
        if(p->state == PROCESS_BLOCKED && p->wait_reason == WAIT_KEY) { 
            p->state = PROCESS_READY; 
            p->wait_reason = WAIT_NONE; 
            return; 
        }
        p = p->next; 
    }
}

void scheduler_start(void) {
    if (!current || !current->next) return;

    process_t *next = current->next;
    next->state = PROCESS_RUNNING;
    current = next;

    extern void set_gs_base(uint64_t base); // <=== To remove (soon)
    // set_gs_base((uint64_t)current); 

    if(next->page_table) { 
        tss_set_kernel_stack(next->kernel_stack); 
    }

    context_switch(&boot_context, next);
}