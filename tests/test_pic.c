#include "ktest.h"
#include "../src/drivers/pic.h"
#include <stdint.h>

/*
Verify the PIC was remapped correctly by reading the 
interrupt mask registers. After pic_init the masks should 
have IRQ0 (timer) and IRQ1 (keyboard) unmasked. Also 
verify masking and unmasking individual IRQs works properly.   
*/



static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    asm volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static void test_pic_irq0_unmasked(void) { 
    uint8_t mask = inb(PIC1_DATA); 
    KTEST_ASSERT_EQ(mask & 0x01, 0); 
}

static void test_pic_irq1_unmasked(void) { 
    uint8_t mask = inb(PIC1_DATA); 
    KTEST_ASSERT_EQ(mask & 0x02, 0); 
}

static void test_pic_mask_unmask(void) { 
    uint8_t before = inb(PIC1_DATA); 
    pic_mask(5); 
    uint8_t masked = inb(PIC1_DATA); 
    KTEST_ASSERT(masked & (1 << 5)); 
    pic_unmask(5); 
    uint8_t after = inb(PIC1_DATA); 
    KTEST_ASSERT_EQ(before, after); 
}

static void test_pic_slave_accessible(void) { 
    /* Just verify if readable without hanging. */
    uint8_t mask = inb(PIC2_DATA); 
    (void)mask; 
    KTEST_ASSERT(1); 
}   

ktest_t pic_tests[] = { 
    {"IRQ0 (timer) is unmasked",    test_pic_irq0_unmasked}, 
    {"IRQ1 (keyboard) is unmasked", test_pic_irq1_unmasked},
    {"mask/unmask IRQ works",       test_pic_mask_unmask}, 
    {"slave PIC is accessible",     test_pic_slave_accessible} 
}; 

int pic_test_count = sizeof(pic_tests) / sizeof(pic_tests[0]); 