#include "framebuffer.h"
#include <stdint.h> 

static volatile uint8_t *fb = (volatile uint8_t *)FB_PHYS; 

static inline void outb(uint16_t port, uint8_t val) { 
    asm volatile("outb %0, %1" :: "a"(val), "Nd"(port)); 
}

static inline uint8_t inb(uint16_t port) { 
    uint8_t v;
    asm volatile("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v; 
}

/**
 * @brief
 */
void fb_init(void) { 
    /* Misc output */
    outb(0x3C2, 0x63);
    
    /* Sequencer */
    static const uint8_t seq[5] = {0x03, 0x01, 0x0F, 0x00, 0x0E}; 
    for(int i = 0; i < 5; i++) { 
        outb(0x3C4, i); 
        outb(0x3C5, seq[i]); 
    }

    /* Unlock CRTC registers 0-7*/
    outb(0x3D4, 0x11); 
    outb(0x3D5, inb(0x3D5) & ~0x80);
    
    /* CRTC */
    static const uint8_t crtc[25] = {
        0x5F,0x4F,0x50,0x82,0x54,0x80,0xBF,0x1F,
        0x00,0x41,0x00,0x00,0x00,0x00,0x00,0x00,
        0x9C,0x0E,0x8F,0x28,0x40,0x96,0xB9,0xA3,0xFF
    };
    for(int i = 0; i < 25; i++) { 
        outb(0x3D4, i); 
        outb(0x3D5, crtc[i]);
    }

    /* Graphics controller */
    static const uint8_t gc[9] = {0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0F,0xFF};
    for(int i = 0; i < 9; i++) { 
        outb(0x3CE, i); 
        outb(0x3CF, gc[i]); 
    }

    /* Attribute controller */
    inb(0x3DA);
    static const uint8_t ac[21] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
        0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
        0x41,0x00,0x0F,0x00,0x00
    };
    for (int i = 0; i < 21; i++) { 
        outb(0x3C0, i); 
        outb(0x3C0, ac[i]);
    }
    outb(0x3C0, 0x20); /* re-enable display */
}

void fb_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    outb(0x3C8, index);
    outb(0x3C9, r >> 2);
    outb(0x3C9, g >> 2);
    outb(0x3C9, b >> 2);
}

void fb_clear(uint8_t colour) {
    for (int i = 0; i < FB_WIDTH * FB_HEIGHT; i++) fb[i] = colour;
}

void fb_put_pixel(int x, int y, uint8_t colour) {
    if ((unsigned)x >= FB_WIDTH || (unsigned)y >= FB_HEIGHT) return;
    fb[y * FB_WIDTH + x] = colour;
}

void fb_draw_rect(int x, int y, int w, int h, uint8_t colour) {
    for (int dy = 0; dy < h; dy++)
        for (int dx = 0; dx < w; dx++)
            fb_put_pixel(x + dx, y + dy, colour);
}