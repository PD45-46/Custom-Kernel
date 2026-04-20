#include "ktest.h"
#include "../src/cpu/idt.h"
#include <stdint.h>

/* 
Can't call exception handlers directly without actually 
triggering the exceptions. Instead verify structural correctness: 
    - IDTR is loaded with the right limit
    - Can register and dispatch handlers correctly
    - Divide by zero is catchable (register a handler and trigger it)
*/

static volatile int divide_by_zero_caught = 0; 

static void test_divide_handler(registers_t* regs) { 
    (void)regs; 
    divide_by_zero_caught = 1; 
}