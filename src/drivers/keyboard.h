#pragma once

void keyboard_init(void); 
int keyboard_has_char(void); /* non zero if a key is waiting */
char keyboard_getchar(void); // last key pressed 