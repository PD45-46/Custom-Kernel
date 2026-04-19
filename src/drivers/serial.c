#include "serial.h"

/*
COM1 is port base address for debugging used by QEMU. 
*/
#define COM1 0x3F8

/* TODO Localise all the outb and inb inline funcs */


static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) { 
    uint8_t val; 
    asm volatile("inb %1, %0" : "=a"(val) : "Nd"(port)); 
    return val; 
}

/**
 * @brief Initialises COM1 at 38400 baud. 
 * 
 */
void serial_init(void) { 
    outb(COM1 + 1, 0x00);  /* disable interrupts            */
    outb(COM1 + 3, 0x80);  /* enable DLAB to set baud rate  */
    outb(COM1 + 0, 0x03);  /* divisor low  = 3 (38400 baud) */
    outb(COM1 + 1, 0x00);  /* divisor high = 0              */
    outb(COM1 + 3, 0x03);  /* 8N1, disable DLAB             */
    outb(COM1 + 2, 0xC7);  /* enable FIFO, clear, 14-byte   */
    outb(COM1 + 4, 0x0B);  /* enable RTS, DSR               */
}

/**
 * @brief Spin wait until transmit buffer is empty. 
 * 
 */
static void serial_wait(void) { 
    while(!(inb(COM1 + 5) & 0x20)); 
}


void serial_putchar(char c) { 
    serial_wait(); 
    outb(COM1, c); 
}

void serial_print(const char *str) { 
    while(*str) serial_putchar(*str++); 
}

void serial_print_hex(uint64_t val) { 
    char buf[17]; 
    buf[16] = '\0';
    for(int i = 15; i >= 0; i--) {
        uint8_t nibble = val & 0xF;
        buf[i] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
        val >>= 4;
    }
    char *p = buf;
    while(*p == '0' && *(p+1) != '\0') p++;
    serial_print(p);
}

void serial_print_int(int64_t val) { 
    if(val < 0) { 
        serial_putchar('-'); 
        val = -val; 
    }
    char buf[21]; 
    buf[20] = '\0'; 
    int i = 19; 
    if(val == 0) { 
        serial_putchar('0'); 
        return; 
    }
    uint64_t u = (uint64_t)val; 
    while(u > 0) { 
        buf[i--] = '0' + (u % 10); 
        u /= 10; 
    }
    serial_print(&buf[i+1]); 
}