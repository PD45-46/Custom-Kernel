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

static unsigned char sc_to_doom(uint8_t sc) {
    switch (sc) {
        case 0x11: return KEY_UPARROW;    /* W */
        case 0x1F: return KEY_DOWNARROW;  /* S */
        case 0x1E: return KEY_LEFTARROW;  /* A */
        case 0x20: return KEY_RIGHTARROW; /* D */
        case 0x39: return KEY_FIRE;       /* Space */
        case 0x1C: return KEY_ENTER;      /* Enter */
        case 0x01: return KEY_ESCAPE;     /* Esc */
        case 0x38: return KEY_LALT;       /* Left Alt */
        case 0x1D: return KEY_RCTRL;      /* Ctrl */
        case 0x16: return KEY_USE;        /* U */
        default:   return 0;
    }
}

int DG_GetKey(int *pressed, unsigned char *doomKey) { 
    raw_key_t event; 
    while(u_get_raw_key(&event)) { 
        unsigned char dk = sc_to_doom(event.scancode); 
        if(!dk) continue;
        *pressed = event.pressed; 
        *doomKey = dk; 
        return 1; 
    }
    return 0; 
}

void DG_SetWindowTitle(const char *title) { 
    (void)title; 
}

uint32_t DG_GetTicksMs(void) { 
    return (uint32_t)(u_gettime() - g_start_ms); 
}