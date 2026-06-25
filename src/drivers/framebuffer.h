#pragma once 
#include <stdint.h>

#define FB_WIDTH  320 
#define FB_HEIGHT 200
#define FB_PHYS   0xA0000ULL

void fb_init(void);
void fb_clear(uint8_t colour);
void fb_put_pixel(int x, int y, uint8_t colour); 
void fb_draw_rect(int x, int y, int w, int h, uint8_t colour); 
void fb_set_palette(uint8_t index, uint8_t r, uint8_t g, uint8_t b); 

#define FB_BLACK   0
#define FB_BLUE    1
#define FB_GREEN   2
#define FB_CYAN    3
#define FB_RED     4
#define FB_WHITE   15
#define FB_YELLOW  14