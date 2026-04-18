#include "vga.h"
#include <stddef.h> 

static uint16_t *vga_buf = (uint16_t *)VGA_ADDRESS; 
static int cursor_x = 0; 
static int cursor_y = 0; 
static uint8_t current_colour = 0;

static inline uint16_t vga_entry(char c, uint8_t colour) { 
    return (uint16_t)c | ((uint16_t)colour << 8); 
}

void vga_clear(void) { 
    for(int i = 0; i < VGA_ROWS * VGA_COLS; i++) { 
        vga_buf[i] = vga_entry(' ', current_colour); 
    }
}

void vga_set_colour(vga_colour_t fg, vga_colour_t bg) { 
    current_colour = (bg << 4) | fg; 
}

static void vga_scroll(void) { 
    for(int y = 0; y < VGA_ROWS - 1; y++) { 
        for(int x = 0; x < VGA_COLS; x++) { 
            vga_buf[y * VGA_COLS + x] = vga_buf[(y+1) * VGA_COLS + x];
        }
    }
    for(int x = 0; x < VGA_COLS; x++) { 
        vga_buf[(VGA_ROWS-1) * VGA_COLS + x] = vga_entry(' ', current_colour);
    } 
    cursor_y = VGA_ROWS - 1;
}

void vga_putchar(char c) { 
    if(c == '\n') { 
        cursor_x = 0; 
        cursor_y++; 
    } else { 
        vga_buf[cursor_y * VGA_COLS + cursor_x] = vga_entry(c, current_colour); 
        cursor_x++; 
        if(cursor_x >= VGA_COLS) { 
            cursor_x = 0; 
            cursor_y++; 
        }
    }
    if(cursor_y >= VGA_ROWS) { 
        vga_scroll(); 
    }
}

void vga_print(const char *str) { 
    while(*str) { 
        vga_putchar(*str++); 
    }
}

void vga_print_hex(uint64_t val) {
    vga_print("0x");
    char buf[17];
    int i = 0;
    if(val == 0) { vga_print("0"); return; }
    while(val) {
        uint8_t nibble = val & 0xF;
        buf[i++] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
        val >>= 4;
    }
    // buf is reversed
    for (int j = i - 1; j >= 0; j--) vga_putchar(buf[j]);
}

void vga_print_uint(uint64_t val) { 
    char buf[21]; 
    buf[20] = '\0'; 
    int i = 19; 
    if(val == 0) { 
        vga_putchar('0'); 
        return; 
    }
    while(val > 0) { 
        buf[i--] = '0' + (val % 10); 
        val /= 10;  
    }
    vga_print(&buf[i+1]); 
}

void vga_print_int(int64_t val) { 
    if(val < 0) { 
        vga_putchar('-'); 
        vga_print_uint((uint64_t)(-val)); 
    } else { 
        vga_print_uint((uint64_t)val); 
    }
}

void vga_init(void) { 
    vga_set_colour(VGA_WHITE, VGA_BLUE); 
    vga_clear(); 
}

