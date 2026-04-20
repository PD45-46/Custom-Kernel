#include "ktest.h"
#include "../src/cpu/gdt.h"
#include <stdint.h> 

/*
Can't insect GDT directly without reading the GDTR register. 
These tests verify if the CPU accepted the GDT by checking that 
segment registers have the expected values after gdt_init. If
the GDT was malformed, gdt_flush would have triple faulted and the 
tests would never be reached.  
*/

static void test_gdt_cs_is_kernel_code(void) { 
    uint16_t cs; 
    asm volatile("mov %%cs, %0" : "=r"(cs));
    /* kernel code selector = 0x08 */
    KTEST_ASSERT_EQ(cs, 0x08); 
}

static void test_gdt_ds_is_kernel_data(void) { 
    uint16_t ds;
    asm volatile("mov %%ds, %0" : "=r"(ds));
    /* kernel data selector = 0x10 */
    KTEST_ASSERT_EQ(ds, 0x10);
}

static void test_gdt_ss_is_kernel_data(void) { 
    uint16_t ss;
    asm volatile("mov %%ss, %0" : "=r"(ss));
    KTEST_ASSERT_EQ(ss, 0x10);
}

/* 
Verify the GDTR limit matches what was set. 
6 entries * 8 bytes each - 1 = 47
*/
static void test_gdt_limit_correct(void) { 
    struct __attribute__((packed)) { uint16_t limit; uint64_t base; } gdtr;
    asm volatile("sgdt %0" : "=m"(gdtr));
    KTEST_ASSERT_EQ(gdtr.limit, (6 * 8) - 1);
}

/*
Verify EFER.SCE is set --- syscall instruction is enabled 
*/
static void test_gdt_syscall_enabled(void) {
    uint32_t low, high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(0xC0000080));
    /* SCE is bit 0 of EFER */
    KTEST_ASSERT(low & 1);
}

/* 
Verify LSTAR MSR points somewhere non-zero 
*/
static void test_gdt_lstar_set(void) {
    uint32_t low, high;
    asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(0xC0000082));
    uint64_t lstar = ((uint64_t)high << 32) | low;
    KTEST_ASSERT(lstar != 0);
}

ktest_t gdt_tests[] = {
    {"CS is kernel code selector", test_gdt_cs_is_kernel_code},
    {"DS is kernel data selector", test_gdt_ds_is_kernel_data},
    {"SS is kernel data selector", test_gdt_ss_is_kernel_data},
    {"GDTR limit is correct",      test_gdt_limit_correct},
    {"EFER.SCE is set",            test_gdt_syscall_enabled},
    {"LSTAR MSR is non-zero",      test_gdt_lstar_set},
};

int gdt_test_count = sizeof(gdt_tests) / sizeof(gdt_tests[0]);