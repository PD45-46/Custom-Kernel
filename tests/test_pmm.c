#include "../src/memory/pmm.h"
#include "ktest.h"


static void test_pmm_alloc_not_null(void) { 
    void *frame = pmm_alloc(); 
    KTEST_ASSERT_NOT_NULL(frame); 
    pmm_free(frame); 
}

ktest_t pmm_tests[] = { 
    {"alloc returns non-null", test_pmm_alloc_not_null}
}; 

int pmm_test_count = sizeof(pmm_tests) / sizeof(pmm_tests[0]); 