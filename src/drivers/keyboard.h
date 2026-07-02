#pragma once
#include <stdint.h> 
typedef struct { 
    uint8_t scancode; // expose raw PS/2 make/break scancodes 
    uint8_t pressed;  // 1 = true, 0 = false 
} key_event_t; 

void keyboard_init(void); 
int keyboard_has_char(void); /* non zero if a key is waiting */
char keyboard_getchar(void); // last key pressed 
int keyboard_get_event(key_event_t *event); 