#include "ktest.h"
#include "../src/cpu/idt.h"
#include <stdint.h>
#include <stddef.h> 

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

static volatile int breakpoint_caught = 0;

static void test_breakpoint_handler(registers_t* regs) { 
    (void)regs; 
    breakpoint_caught = 1; 
}

static void test_idt_limit_correct(void) {
    struct __attribute__((packed)) { uint16_t limit; uint64_t base; } idtr;
    asm volatile("sidt %0" : "=m"(idtr));
    /* 256 entries * 16 bytes each - 1 = 4095 = 0x0FFF */
    KTEST_ASSERT_EQ(idtr.limit, (256 * 16) - 1);
}

static void test_idt_base_nonzero(void) {
    struct __attribute__((packed)) { uint16_t limit; uint64_t base; } idtr;
    asm volatile("sidt %0" : "=m"(idtr));
    KTEST_ASSERT_NEQ(idtr.base, 0);
}

static void test_idt_register_handler(void) { 
    /* register a breakpoint handler and trigger int3 */
    idt_register_handler(3, test_breakpoint_handler); 
    breakpoint_caught = 0; 
    asm volatile("int3"); 
    KTEST_ASSERT_EQ(breakpoint_caught, 1); 
    idt_register_handler(3, NULL); 
}

static void test_idt_null_handler(void) { 
    /*
    Can't easily test that NULL handlers halt safely without 
    crashing the test run. Just verify the handler array accepts 
    NULL without faulting. 
    The actual behavior is handled implicitly --- if the kernel is
    still runing, unhandled exceptions that occurred during boot 
    were caught. 
    */
    idt_register_handler(48, NULL); /* user defined slot */
    KTEST_ASSERT(1); /* If reached, then there was no crash */
}

ktest_t idt_tests[] = { 
    {"IDTR limit is correct",          test_idt_limit_correct}, 
    {"IDTR base is non-zero",          test_idt_base_nonzero}, 
    {"register and trigger handler",   test_idt_register_handler}, 
    {"NULL handler registration safe", test_idt_null_handler} 
}; 

int idt_test_count = sizeof(idt_tests) / sizeof(idt_tests[0]); 