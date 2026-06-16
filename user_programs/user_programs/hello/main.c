#include "../../user/user_lib.h"

void _start(void) { 
    char msg[] = { 
        'H', 'e', 'l', 'l', 'o', ' ', 
        'f', 'r', 'o', 'm', ' ', 
        'E', 'L', 'F', '!', '\n'
    }; 
    u_write(msg, 16); 
    u_exit(); 
}