#include "../src/memory/pmm.h"
#include "ktest.h"


static void test_pmm_alloc_not_null(void) { 
    void *frame = pmm_alloc(); 
    KTEST_ASSERT_NOT_NULL(frame); 
    pmm_free(frame); 
}

static void test_pmm_alloc_aligned(void) { 
    void *frame = pmm_alloc(); 
    KTEST_ASSERT_NOT_NULL(frame); 
    // all frames must be page aligned 
    KTEST_ASSERT(((uint64_t)frame & 0xFFF) == 0); 
    pmm_free(frame); 
}

static void test_pmm_free_reclaims(void) { 
    size_t before = pmm_free_frames(); 
    void *frame = pmm_alloc(); 
    KTEST_ASSERT(pmm_free_frames() == before - 1); 
    pmm_free(frame); 
    KTEST_ASSERT_EQ(pmm_free_frames(), before); 
}

static void test_pmm_multiple_allocs_unique(void) { 
    void *a = pmm_alloc(); 
    void *b = pmm_alloc(); 
    KTEST_ASSERT_NOT_NULL(a);
    KTEST_ASSERT_NOT_NULL(b);

    KTEST_ASSERT_NEQ(a, b);  
    pmm_free(a); 
    pmm_free(b); 
}



ktest_t pmm_tests[] = { 
    {"alloc returns non-null",        test_pmm_alloc_not_null}, 
    {"alloc returns aligned address", test_pmm_alloc_aligned}, 
    {"free reclaims frame",           test_pmm_free_reclaims}, 
    {"multiple allocs are unique",    test_pmm_multiple_allocs_unique}
}; 

int pmm_test_count = sizeof(pmm_tests) / sizeof(pmm_tests[0]); 