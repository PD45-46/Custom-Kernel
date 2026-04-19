#pragma once
#include "../src/drivers/vga.h"
#include "../src/drivers/serial.h"

/*
Kernel assertion. If condition is false, prints file/line/condition 
and then halts. This will be used everywhere an invariant must hold. 
*/

#define KASSERT(cond) \
    do { \
        if (!(cond)) { \
            serial_print("[ASSERT FAILED] "); \
            serial_print(__FILE__); \
            serial_print(":"); \
            serial_print_int(__LINE__); \
            serial_print(" -- "); \
            serial_print(#cond); \
            serial_print("\n"); \
            vga_print("[ASSERT FAILED] " #cond "\n"); \
            for(;;) asm volatile("hlt"); \
        } \
    } while(0)

/* Common case of checking a pointer */
#define KASSERT_NOT_NULL(ptr) KASSERT((ptr) != NULL)