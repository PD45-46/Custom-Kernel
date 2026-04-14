#include "timer.h"
#include "pic.h"
#include "../cpu/idt.h"
#include "../drivers/vga.h"
#include <stdint.h> 

static uint64_t ticks = 0; 

static inline void outb(uint16_t port, uint8_t val) { 
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

/**
 * @brief Called on every timer interrupt. Increments tick counter 
 *        and sends EOI so the PIC allows t he next timer interrupt 
 *        to fire. 
 * 
 * @param regs 
 */
static void timer_handler(registers_t *regs) { 
    (void)regs; 
    ticks++; 
    pic_send_eoi(0); 
}

/**
 * @brief Programs the PIT to fire at given frequency (Hz).
 *        PIT base frequency is 1193182 Hz. 
 * 
 * @param frequency Desired frequency of the timer 
 */
void timer_init(uint32_t frequency) { 
    uint32_t divisor = 1193182 / frequency; 

    outb(0x43, 0x36); 
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);

    idt_register_handler(32, timer_handler); 
    pic_unmask(0); 
}

uint64_t timer_ticks() { 
    return ticks; 
}

