#include "ktest.h"
#include "../src/process/process.h"

/* 
TODO 
AFTER ADDING FUNCTIONALITY WHERE DESTROYED PROCESSES WILL BE REMOVED FROM 
MEMORY, MAKE TESTS TO ENSURE THAT HAPPENS. ALSO WRITE TEST CASES FOR OTHER 
STATES THAN JUST PROCESS_READY.
*/

static void dummy_entry(void) { 
    for(;;) asm volatile("hlt"); 
}

static void test_process_create_not_null(void) { 
    process_t *p = process_create(dummy_entry);
    KTEST_ASSERT_NOT_NULL(p); 
    process_destroy(p);  
}

static void test_process_valid_pid(void) { 
    process_t *p = process_create(dummy_entry); 
    KTEST_ASSERT_NOT_NULL(p); 
    KTEST_ASSERT(p->pid > 0); 
    process_destroy(p); 
}

static void test_process_inital_state_ready(void) { 
    process_t *p = process_create(dummy_entry); 
    KTEST_ASSERT_NOT_NULL(p); 
    KTEST_ASSERT_EQ(p->state, PROCESS_READY); 
    process_destroy(p); 
}

static void test_process_pid_unique(void) { 
    process_t *a = process_create(dummy_entry);
    process_t *b = process_create(dummy_entry); 
    KTEST_ASSERT_NOT_NULL(a); 
    KTEST_ASSERT_NOT_NULL(b); 
    KTEST_ASSERT_NEQ(a->pid, b->pid); 
    process_destroy(a); 
    process_destroy(b); 
}

static void test_process_has_stack(void) { 
    process_t *p = process_create(dummy_entry); 
    KTEST_ASSERT_NOT_NULL(p); 
    KTEST_ASSERT_NEQ(p->kernel_stack, 0); 
    KTEST_ASSERT_NEQ(p->context.rsp, 0); 
    process_destroy(p); 
}




ktest_t process_tests[] = { 
    {"create process is non-null", test_process_create_not_null}, 
    {"process has valid pid",      test_process_valid_pid},
    {"process state is READY",     test_process_inital_state_ready},
    {"process IDs are unique",     test_process_pid_unique}, 
    {"process has kernel stack",   test_process_has_stack}
}; 

int process_test_count = sizeof(process_tests) / sizeof(process_tests[0]); 