#include "syscall.h"
#include "../drivers/vga.h"
#include "../process/scheduler.h"
#include "../process/process.h" 
#include <stdint.h> 


/*

*/


// TODO LOOK AT NOTES.
/**
 * @brief Write a string to VGA and serial. 
 *        Serial something that will be developed in the future for testing... 
 * 
 * @param str_ptr Pointer to the string (user virtual address)
 * @param len Length of string 
 * @return int64_t Number of bits written or negative value for error. 
 * @note In a real kernel str_ptr must be validated that it points to the 
 *       the user space before doing anything. That will be skipped for now. 
 *       
 */
static int64_t sys_write(uint64_t str_ptr, uint64_t len) { 
    const char *str = (const char *)str_ptr;
    if(!str) return -1; 
    
    for(uint64_t i = 0; i < len; i++) { 
        vga_putchar(str[i]); 
        /* SERIAL PLACEHOLDER */
    } 
    return (int64_t)len; 

}

/**
 * @brief Terminate the current process. Marks teh current process as
 *        dead. The scheduler will skip it on the next tick.  
 * 
 * 
 * @param exit_code (Ignored for now)
 * @return int64_t 
 * @note Eventually the memory for the dead process will nee to be
 *       cleaned up --- for now it will just be marked as dead. 
 */
static int64_t sys_exit(uint64_t exit_code) { 
    (void)exit_code; 
    process_t *curr = scheduler_current(); 
    if(curr) { 
        curr->state = PROCESS_DEAD; 
    }
    scheduler_tick(); 
    for(;;) asm volatile("hlt"); 
    return 0;
}

/**
 * @brief Voluntarily give up the CPU. Calls scheduler_tick directly. 
 *        The current process goes back to ready state and the next one runs. 
 * 
 * @return int64_t 
 * @note Declare error states instead of just returning 0. 
 */
static int64_t sys_yield(void) { 
    scheduler_tick(); 
    /* TODO */
    return 0; 
}

/**
 * @brief Gets the current process ID. 
 * 
 * @return uint64_t ID or -1 on error. 
 */
static uint64_t sys_getpid(void) { 
    process_t *curr = scheduler_current(); 
    if(!curr) return -1; 
    return (uint64_t)curr->pid; 
}

/**
 * @brief 
 * 
 * @param num 
 * @param arg1 
 * @param arg2 
 * @param arg3 
 * @return int64_t 
 */
int64_t syscall_dispatch(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3) { 
    /* Serials... */
    (void)arg3; 
    switch(num) { 
        case SYS_WRITE: return sys_write(arg1, arg2); 
        case SYS_EXIT:  return sys_exit(arg1); 
        case SYS_YIELD: return sys_yield(); 
        case SYSGETPID: return sys_getpid(); 
        default: 
            /* Serials... */
            return -1; 
    }


}