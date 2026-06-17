#include "../../src/user/user_lib.h"
#include "malloc.h"

void _start(void) {
    char *buf = (char *)malloc(64); 
    if(!buf) { 
        char err[] = {'m','a','l','l','o','c',' ','f','a','i','l','\n'}; 
        u_write(err, 12);
        u_exit(); 
    }
    buf[0]='H'; buf[1]='e'; buf[2]='l'; buf[3]='l'; buf[4]='o';
    buf[5]=' '; buf[6]='h'; buf[7]='e'; buf[8]='a'; buf[9]='p';
    buf[10]='!'; buf[11]='\n';

    u_write(buf, 12);
    free(buf);
    u_exit();
}

