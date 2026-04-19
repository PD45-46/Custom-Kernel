#pragma once
#include "../src/drivers/serial.h"

typedef void (*test_fn_t)(void); 

typedef struct { 
    const char *name; 
    test_fn_t fn; 
} ktest_t; 

/* Set by kTEST_ASSERT inside test functions */
extern int ktest_current_failed;
extern int ktest_pass_count;
extern int ktest_fail_count;

#define KTEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            serial_print("  [FAIL] "); \
            serial_print(#cond); \
            serial_print(" at line "); \
            serial_print_int(__LINE__); \
            serial_print("\n"); \
            ktest_current_failed = 1; \
        } \
    } while(0)

#define KTEST_ASSERT_EQ(a, b)  KTEST_ASSERT((a) == (b))
#define KTEST_ASSERT_NEQ(a, b) KTEST_ASSERT((a) != (b))
#define KTEST_ASSERT_NULL(p)   KTEST_ASSERT((p) == NULL)
#define KTEST_ASSERT_NOT_NULL(p) KTEST_ASSERT((p) != NULL)

void ktest_run(const char *suite_name, ktest_t *tests, int count);