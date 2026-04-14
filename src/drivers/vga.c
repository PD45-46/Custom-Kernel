#include "vga.h"
#include <stddef.h> 

static uint16_t *vga_buf = (uint16_t *)VGA_ADDRESS; 
static int cursor_x = 0; 
static int cursor_y = 0; 
static uint8_t current_colour = 0;

static inline uint16_t vga_entry(char c, uint8_t colour) { 
    return (uint16_t)c | ((uint16_t)colour << 8); 
}

void vga_init() { 
    current_colour = (VGA_BLACK << 4) | VGA_LIGHT_CYAN; 
    vg_clear(); 
}

void vga_clear() { 
    for(int y = 0; y < VGA_ROWS; y++) { 
        for(int x = 0; x < VGA_COLS; x++) { 
            vga_buf[y * VGA_COLS + x] = vga_entry(' ', current_colour); 
        }
    }
    cursor_x = 0; 
    cursor_y = 0; 
}

void vga_set_colour(vga_colour_t fg, vga_colour_t bg) { 
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

static void vga_scroll() { 
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

