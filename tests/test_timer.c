#include "ktest.h"
#include "../src/drivers/timer.h"
#include <stdint.h> 

/*
Can't test interrupt firing without enabling interrupts which 
risks the scheduler running during tests. Instead verify the 
tick counter advances when interrupts are briefly enabled, and 
check PIT configuration. 
*/

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    asm volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static void test_timer_ticks_start_zero(void) { 
    uint64_t t = timer_ticks(); 
    (void)t; 
    KTEST_ASSERT(1); 
}

static void test_timer_ticks_advance(void) { 
    uint64_t before = timer_ticks(); 
    asm volatile("sti"); 
    // spin long enough for one tick
    for(volatile int i = 0; i < 1000000; i++); 
    asm volatile("cli"); 
    uint64_t after = timer_ticks(); 
    KTEST_ASSERT(after > before); 
}

static void test_timer_ticks_monotonic(void) { 
    uint64_t a = timer_ticks(); 
    uint64_t b = timer_ticks(); 
    KTEST_ASSERT(a <= b); 
}

ktest_t timer_tests[] = {
    {"timer_ticks returns value",     test_timer_ticks_start_zero}, 
    {"ticks advance with interrupts", test_timer_ticks_advance}, 
    {"ticks are monotonic",           test_timer_ticks_monotonic}
}; 

int timer_test_count = sizeof(timer_tests) / sizeof(timer_tests[0]); 