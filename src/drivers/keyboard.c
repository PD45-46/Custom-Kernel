#include "keyboard.h"
#include "pic.h"
#include "../cpu/idt.h"
#include "../drivers/vga.h"
#include "../process/scheduler.h"

#define KB_BUFFER_SIZE 64 
#define KB_EVENT_BUF 32

static char kb_buf[KB_BUFFER_SIZE];
static key_event_t key_events[KB_EVENT_BUF]; 

static int kb_read_pos = 0; 
static int kb_write_pos = 0;

static int event_head = 0; 
static int event_tail = 0;

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

static inline uint8_t inb(uint16_t port) { 
    uint8_t val; 
    asm volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val; 
}

/**
 * @brief Called on every keypress. Read the scancode from port 0x60. 
 *        Those above 0x60 are keyrelease events (ignored). Translate 
 *        the scancode to ASCII via scancode_table and print via vga. 
 * 
 * @param regs 
 */
static void keyboard_handler(registers_t *regs) {

    uint8_t sc = inb(0x60); 
    key_event_t event; 
    event.pressed = !(sc & 0x80); 
    event.scancode = sc & 0x7F; 
    int next = (event_tail + 1) % KB_EVENT_BUF; 
    if(next != event_head) { 
        key_events[event_tail] = event; 
        event_tail = next; 
    }

    if(sc & 0x80) { 
        pic_send_eoi(1); 
        return; 
    }


    (void)regs; 
    uint8_t scancode; 
    asm volatile("inb $0x60, %0" : "=a"(scancode));

    if(scancode < 128) { 
        char c = scancode_table[scancode]; 
        if(c) { 
            // last_key = c; 
            // vga_putchar(c); 
            int next = (kb_write_pos + 1) % KB_BUFFER_SIZE; 
            if(next != kb_read_pos) { /* Drop if full */
                kb_buf[kb_write_pos] = c; 
                kb_write_pos = next;  
            }
            vga_putchar(c); 
            scheduler_wake_key_waiter(); 
        }
    }
    pic_send_eoi(1);  // IRQ  1 
}

void keyboard_init(void) { 
    idt_register_handler(33, keyboard_handler); 
    pic_unmask(1); 
}

int keyboard_has_char(void) { 
    return kb_read_pos != kb_write_pos; 
}

/**
 * @brief Gets and resets the last key used. 
 * 
 * @return char Last key. 
 */
char keyboard_getchar(void) { 
    if(kb_read_pos == kb_write_pos) return 0; 
    char c = kb_buf[kb_read_pos]; 
    kb_read_pos = (kb_read_pos + 1) % KB_BUFFER_SIZE; 
    return c; 
}

int keyboard_get_event(key_event_t *event) { 
    if(event_head == event_tail) return 0; 
    *event = key_events[event_head]; 
    event_head = (event_head + 1) % KB_EVENT_BUF; 
    return 1; 
}





