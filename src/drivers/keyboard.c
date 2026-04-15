#include "keyboard.h"
#include "pic.h"
#include "../cpu/idt.h"
#include "../drivers/vga.h"

static char last_key = 0; 

/* 
US QWERTY scancode to ASCII translation table. 
*/
static const char scancode_table[128] = { 
    0, 0, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,
    '*',0,' '
};

/**
 * @brief Called on every keypress. Read the scancode from port 0x60. 
 *        Those above 0x60 are keyrelease events (ignored). Translate 
 *        the scancode to ASCII via scancode_table and print via vga. 
 * 
 * @param regs 
 */
static void keyboard_handler(registers_t *regs) { 
    (void)regs; 

    uint8_t scancode; 
    asm volatile("inb $0x60, %0" : "=a"(scancode));

    if(scancode < 128) { 
        char c = scancode_table[scancode]; 
        if(c) { 
            last_key = c; 
            vga_putchar(c); 
        }
    }
    pic_send_eoi(1);  // IRQ  1 
}

void keyboard_init() { 
    idt_register_handler(33, keyboard_handler); 
    pic_unmask(1); 
}

/**
 * @brief Gets and resets the last key used. 
 * 
 * @return char Last key. 
 */
char keyboard_getchar() { 
    char c = last_key; 
    last_key = 0; 
    return last_key; 
}



