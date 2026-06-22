#include "game/doomgeneric/doomgeneric.h"
#include "game/doomgeneric/doomkeys.h"
#include "game/doomgeneric/doomtype.h"
#include "../../src/user/user_lib.h"

#include <stdint.h> 

extern byte *I_VideoBuffer;
static uint8_t *g_fb = (uint8_t *)0; 
static uint64_t g_start_ms = 0; 

void DG_Init(void) { 
    g_fb = (uint8_t *)u_map_fb(); 
    g_start_ms = u_gettime(); 
}

void DG_SetPalette(byte *palette) { 
    u_setpalette((const uint8_t *)palette);
}

void DG_DrawFrame(void) { 
    if(!g_fb) return; 
    uint8_t *src = (uint8_t *)I_VideoBuffer; 
    for(int i = 0; i < DOOMGENERIC_RESX * DOOMGENERIC_RESY; i++) { 
        g_fb[i] = src[i]; 
    }
    // for (int i = 0; i < 320 * 200; i++) g_fb[i] = 4;
} 

void DG_SleepMs(uint32_t ms) { 
    uint64_t ticks = ms / 10; 
    if(!ticks) ticks = 1; 
    u_sleep(ticks); 
}

static unsigned char translate_key(char c) { 
    if(c == 'w') return KEY_UPARROW; 
    if(c == 's') return KEY_DOWNARROW; 
    if(c == 'a') return KEY_LEFTARROW; 
    if(c == 'd') return KEY_RIGHTARROW; 
    if(c == ' ') return KEY_FIRE; 
    if(c == '\n') return KEY_ENTER; 
    if(c == 27) return KEY_ESCAPE; 
    return (unsigned char)c; 
}

int DG_GetKey(int *pressed, unsigned char *doomKey) { 
    char c = u_getkey(); 
    if(!c) return 0; 
    *pressed = 1; 
    *doomKey = translate_key(c); 
    return 1; 
}

void DG_SetWindowTitle(const char *title) { 
    (void)title; 
}

uint32_t DG_GetTicksMs(void) { 
    return (uint32_t)(u_gettime() - g_start_ms); 
}