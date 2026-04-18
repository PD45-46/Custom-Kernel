#pragma once
#include <stdint.h> 

#define SYS_WRITE 0 
#define SYS_EXIT 1
#define SYS_YIELD 2
#define SYSGETPID 3

void syscall_init(); 
/** 
 * @brief Dispatches the correct kernel function depending on the inputs given. 
 * @param num which syscall to use, e.g: SYS_WRITE, SYS_EXIT...  
 * @param a1 argument 1 passed to syscall 
 * @param a2 argument 2 passed to syscall 
 * @param a3 argument 3 passed to syscall 
 * @return Different for each case: 
 *  - SYS_WRITE: Number of bytes written 
 *  - SYS...  
 */
uint64_t syscall_handler(uint64_t num, uint64_t a1, uint64_t a2, uint64_t a3); 