#include "../../src/user/user_lib.h"
#include "malloc.h"
#include "../../src/drivers/framebuffer.h"

/* TODO REMOVE MAGIC NUMBERS */

void _start(void) {
    uint8_t *fb = u_map_fb();
    if ((int64_t)fb == -1) {
        u_exit();
    }
    for (int i = 0; i < 320 * 200; i++) fb[i] = 0;
    for (int y = 0;   y < 66;  y++)
        for (int x = 0; x < 320; x++)
            fb[y * 320 + x] = 4;
    for (int y = 66;  y < 133; y++)
        for (int x = 0; x < 320; x++)
            fb[y * 320 + x] = 2;
    for (int y = 133; y < 200; y++)
        for (int x = 0; x < 320; x++)
            fb[y * 320 + x] = 1;

    u_sleep(300);
    u_exit();
}

