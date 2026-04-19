#include "ktest.h"
#include "../src/memory/heap.h"

// TODO TEST coalescing and first fit strategies 

static void test_kmalloc_not_null(void) { 
    void *p = kmalloc(64); 
    KTEST_ASSERT_NOT_NULL(p);
    kfree(p);  
}

static void test_kmalloc_unique(void) { 
    void *a = kmalloc(64); 
    void *b = kmalloc(64); 
    KTEST_ASSERT_NOT_NULL(a); 
    KTEST_ASSERT_NOT_NULL(b);
    
    KTEST_ASSERT_NEQ(a, b); 
    kfree(a); 
    kfree(b); 
}

static void test_kmalloc_write_read(void) { 
    uint8_t *p = kmalloc(256); 
    KTEST_ASSERT_NOT_NULL(p); 

    for(int i = 0; i < 256; i++) p[i] = (uint8_t)i; 
    for(int i = 0; i < 256; i++) KTEST_ASSERT_EQ(p[i], (uint8_t)i); 

    kfree(p); 
}


static void test_kcalloc_zeroed(void) { 
    uint8_t *p = kcalloc(1, 256); 
    KTEST_ASSERT_NOT_NULL(p); 
    for(int i = 0; i < 256; i++) KTEST_ASSERT_EQ(p[i], 0); 
    kfree(p); 
}

static void test_kfree_then_realloc(void) { 
    void *a = kmalloc(64); 
    kfree(a); 
    void *b = kmalloc(64); 
    KTEST_ASSERT_NOT_NULL(b); 
    kfree(b); 
}
 

ktest_t heap_tests[] = { 
    {"kmalloc returns non-null",        test_kmalloc_not_null}, 
    {"kmalloc returns unique pointers", test_kmalloc_unique}, 
    {"read/write allocated memory",     test_kmalloc_write_read}, 
    {"kcalloc is zeroed",               test_kcalloc_zeroed}, 
    {"free then realloc memory",        test_kfree_then_realloc}
}; 

int heap_test_count = sizeof(heap_tests) / sizeof(heap_tests[0]); 