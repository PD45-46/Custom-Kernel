#include "pic.h"
#include <stdint.h> 

/**
 * @brief Write a byte from an I/O port. Hardware devices are accessed 
 *        via I/O ports in x86 rather than memory addresses.  
 * 
 * @param port 
 * @param val 
 */
static inline void outb(uint16_t port, uint8_t val) { 
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

/**
 * @brief Read a byte from an I/O port. 
 * 
 * @param port 
 * @return uint8_t 
 */
static inline uint8_t inb(uint16_t port) { 
    uint8_t val; 
    asm volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val; 
}

/* tactic to waste a small amount of time by writing to */
static inline void io_wait(void) { 
    outb(0x80, 0); 
} 


/**
 * @brief Remaps the PIC so IRQs start on IDT entry 32. 
 *        The PIC initialisation sequence is a fixed protocol: 
 *          Send ICW1 (init command) then ICW2 (vector offset), then ICW3 
 *          (cascade info) then ICW4 (mode). Both master (PIC1) and slave 
 *          (PIC2) need this. 
 * 
 */
void pic_init(void) { 
    // save current masks 
    uint8_t mask1 = inb(PIC1_DATA); 
    uint8_t mask2 = inb(PIC2_DATA); 

    // ICW1 
    outb(PIC1_CMD, 0x11); io_wait(); 
    outb(PIC2_CMD, 0x11); io_wait(); 

    // ICW2 
    outb(PIC1_DATA, 0x20); io_wait(); 
    outb(PIC2_DATA, 0x28); io_wait(); 

    // ICW3 
    outb(PIC1_DATA, 0x04); io_wait(); 
    outb(PIC2_DATA, 0x02); io_wait(); 

    // ICW4
    outb(PIC1_DATA, 0x01); io_wait(); 
    outb(PIC2_DATA, 0x01); io_wait(); 

    // restore marks 
    outb(PIC1_DATA, mask1); 
    outb(PIC2_DATA, mask2); 
}


/**
 * @brief Tells the PIC the interrupt has been handled. Needs to 
 *        be called at the end of every hardware interrupt handler, else
 *        PICs of that interrupt type will not be sent anymore. Note to 
 *        always tell masker PIC. 
 * 
 * @param irq 
 */
void pic_send_eoi(uint8_t irq) { 
    if(irq >= 8) { 
        outb(PIC2_CMD, 0x20); 
    }
    outb(PIC1_CMD, 0x20); 
}

void pic_mask(uint8_t irq) { 
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA; 
    if(irq >= 8) { 
        irq -= 8; 
    }
    outb(port, inb(port) | (1 << irq)); 
}

void pic_unmask(uint8_t irq) { 
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    if(irq >= 8) { 
        irq -= 8; 
    }
    outb(port, inb(port) & ~(1 << irq)); 
}

