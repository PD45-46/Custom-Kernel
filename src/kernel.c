// #include "kernel.h"
#include "drivers/serial.h"
#include "drivers/vga.h"
#include "cpu/gdt.h"
#include "cpu/idt.h"
#include "drivers/pic.h"
#include "drivers/timer.h"
#include "drivers/keyboard.h"
#include "memory/pmm.h"
#include "memory/vmm.h"
#include "memory/heap.h"
#include "process/scheduler.h"
#include "process/process.h"
#include "user/user_lib.h"
#include "drivers/framebuffer.h"
#include "filesystem/ramdisk.h"
#include "filesystem/elf.h"
#include <stdlib.h> 


#ifdef RUN_TESTS 
#include "../tests/ktest.h"

extern ktest_t pmm_tests[];  extern int pmm_test_count;
extern ktest_t heap_tests[]; extern int heap_test_count; 
extern ktest_t process_tests[]; extern int process_test_count; 
extern ktest_t gdt_tests[]; extern int gdt_test_count; 
extern ktest_t idt_tests[]; extern int idt_test_count; 
extern ktest_t pic_tests[]; extern int pic_test_count; 
extern ktest_t timer_tests[]; extern int timer_test_count; 





static void run_all_tests(void) {
    serial_print("\n=== RUNNING KERNEL TESTS ===\n");
    ktest_run("PMM", pmm_tests, pmm_test_count);
    ktest_run("HEAP", heap_tests, heap_test_count); 
    ktest_run("PROCESS", process_tests, process_test_count); 
    ktest_run("GDT", gdt_tests, gdt_test_count); 
    ktest_run("IDT", idt_tests, idt_test_count); 
    ktest_run("PIC", pic_tests, pic_test_count); 
    ktest_run("TIMER", timer_tests, timer_test_count); 




    serial_print("=== TESTS COMPLETE ===\n");
}

#endif


// TODO MOVE 
#define SYSCALL_STACK_SIZE 16384 
static uint8_t syscall_stack[SYSCALL_STACK_SIZE] __attribute__((aligned(16)));
uint64_t kernel_stack_top = (uint64_t)(syscall_stack + SYSCALL_STACK_SIZE);

void process_A(void) { 
    while(1) { 
        vga_print("A "); 
        asm volatile("sti\n\thlt"); 
    }
}

void process_B(void) { 
    uint64_t tick = 0; 
    while(1) { 
        tick++; 
        if(tick % 18 == 0) { 
            vga_print("B"); 
        }
        asm volatile("sti\n\thlt"); 
    }

}

void process_C(void) {
    int arr[6] = {1, 2, 3, 4, 5, 6};   
    while(1) { 
        uint64_t t = timer_ticks(); 
        vga_print_int(arr[t % 6] + 1); 
        asm volatile("sti\n\thlt"); 
    } 
}

/**
 * Rules for Ring 3 code. 
 *  - No string literals (they live in the kernel .rodata)
 *  - No kernel function calls (wrong addresses + ring 3 cant access)
 *  - Build strings on the stack byte by byte 
 *  - Communicate only via syscalls 
 * 
 */
void user_process(void) { 
    char msg[5]; 
    msg[0]='R'; msg[1]='3'; msg[2]='!'; msg[3]=' '; msg[4]='\0';

    while(1) { 
        u_write(msg, 3); 

        /* All caller-saved regs in clobber list — compiler must use
           a callee-saved reg (rbx/r12-r15) for i, which syscall_entry preserves */
        u_yield(); 
    }
}

void user_process_A(void) { 
    char buf[64]; 
    char p[] = {'>', ' ', 0}; 
    u_write(p, 2); 
    int64_t n = u_read(buf, sizeof(buf) - 1); 
    buf[n] = 0; 
    u_write(buf, n); 
    u_exit(); 
}

void pong(void) {
    uint8_t *fb = u_map_fb();

    int ball_x = 160, ball_y = 100;
    int ball_dx = 2,  ball_dy = 1;
    int p1_y = 80, p2_y = 80;

    const int PAD_W = 4, PAD_H = 30, PAD_SPEED = 2;
    const int BALL_SIZE = 4;

    while (1) {
        char k = u_getkey();

        /* left paddle: W / S */
        if (k == 'w' && p1_y > 0)              p1_y -= PAD_SPEED;
        if (k == 's' && p1_y < 200 - PAD_H)    p1_y += PAD_SPEED;

        /* right paddle: UP / DOWN arrow */
        if (k == 'i' && p2_y > 0)            p2_y -= PAD_SPEED;
        if (k == 'k' && p2_y < 200 - PAD_H)  p2_y += PAD_SPEED;

        /* physics */
        ball_x += ball_dx;
        ball_y += ball_dy;

        if (ball_y <= 0 || ball_y >= 200 - BALL_SIZE) ball_dy = -ball_dy;

        if (ball_x <= 8 + PAD_W &&
            ball_y + BALL_SIZE >= p1_y && ball_y <= p1_y + PAD_H)
            ball_dx =  2;

        if (ball_x >= 320 - 8 - PAD_W - BALL_SIZE &&
            ball_y + BALL_SIZE >= p2_y && ball_y <= p2_y + PAD_H)
            ball_dx = -2;

        /* ball out — show red square, pause, reset */
        if (ball_x < 0 || ball_x > 320) {
            for (int i = 0; i < 320 * 200; i++) fb[i] = 0;
            for (int gy = 70; gy < 130; gy++)
                for (int gx = 120; gx < 200; gx++)
                    fb[gy * 320 + gx] = 4;   /* red */
            u_sleep(150);                     /* hold ~1.5 s */
            ball_x = 160; ball_y = 100;
            ball_dx = (ball_dx > 0) ? -2 : 2;
            continue;                         /* skip normal draw this frame */
        }

        /* draw */
        for (int i = 0; i < 320 * 200; i++) fb[i] = 0;

        for (int y = 0; y < 200; y += 6) {
            int base = y * 320 + 159;
            fb[base] = 7; fb[base+1] = 7;
        }

        for (int dy = 0; dy < PAD_H; dy++)
            for (int dx = 0; dx < PAD_W; dx++) {
                fb[(p1_y + dy) * 320 + (8   + dx)] = 15;
                fb[(p2_y + dy) * 320 + (308 + dx)] = 15;
            }

        for (int dy = 0; dy < BALL_SIZE; dy++)
            for (int dx = 0; dx < BALL_SIZE; dx++)
                fb[(ball_y + dy) * 320 + (ball_x + dx)] = 14;

        u_sleep(3);
    }
}

void file_test(void) { 
    /* /hello.txt on the stack */
    char path[12]; 
    path[0]='/'; path[1]='h'; path[2]='e'; path[3]='l'; path[4]='l';
    path[5]='o'; path[6]='.'; path[7]='t'; path[8]='x'; path[9]='t';
    path[10]='\0';

    int fd = u_open(path);
    if (fd < 0) { u_exit(); return; }

    char buf[64];
    int  n = u_fread(fd, buf, 63);
    u_fclose(fd);

    if (n > 0) u_write(buf, (uint64_t)n);
    u_exit();
}


void kernel_main(void) { 
    serial_init();
    vga_init();
    vga_print("Kernel Booting...\n");

    gdt_init();        vga_print("[OK] GDT\n");
    idt_init();        vga_print("[OK] IDT\n");
    pic_init();        vga_print("[OK] PIC\n");
    
    keyboard_init();   vga_print("[OK] Keyboard\n");
    pmm_init();        vga_print("[OK] PMM\n");
    vmm_init();        vga_print("[OK] VMM\n");
    heap_init();       vga_print("[OK] Heap\n");

    timer_init(100);   vga_print("[OK] Timer\n");
    scheduler_init();  vga_print("[OK] Scheduler\n");
    syscall_init();    vga_print("[OK] Syscall\n");
    fb_init();         vga_print("[OK] FrameBuf");
    ramdisk_init();    vga_print("[OK] Ramdisk");




    
    #ifdef RUN_TESTS
        vga_print("[RUNNING TESTS]\n");
        asm volatile("cli"); 
        run_all_tests();
        asm volatile("sti"); 
        vga_print("[TESTS DONE]\n");
    #endif

    /* 1. Allow user space to write FS.base (needed for musl TLS) */
    uint64_t cr4;
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    cr4 |= (1ULL << 16);   /* CR4.FSGSBASE */

    /* 2. Enable SSE Extensions (Crucial for musl-gcc / Doom) */
    cr4 |= (1ULL << 9);    /* CR4.OSFXSR: Enable fxsave/fxrstor */
    cr4 |= (1ULL << 10);   /* CR4.OSXMMEXCPT: Enable unmasked SIMD exceptions */
    asm volatile("mov %0, %%cr4" :: "r"(cr4));

    uint64_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~(1ULL << 2);   /* Clear CR0.EM (Emulation bit) */
    cr0 |= (1ULL << 1);    /* Set CR0.MP (Monitor Coprocessor) */
    asm volatile("mov %0, %%cr0" :: "r"(cr0));

    process_t *a = process_create(process_A); scheduler_add(a);
    // process_t *b = process_create(process_B); scheduler_add(b);  
    // process_t *c = process_create(process_C); scheduler_add(c); 
    // process_t *u = process_create_user(user_process); scheduler_add(u); 
    // process_t *u_a = process_create_user(user_process_A); scheduler_add(u_a);
    // vga_print("Starting scheduler...\n");

    // fb_clear(FB_BLACK); 
    // fb_draw_rect(0,   0,   160, 100, FB_RED);
    // fb_draw_rect(160, 0,   160, 100, FB_GREEN);
    // fb_draw_rect(0,   100, 160, 100, FB_BLUE);
    // fb_draw_rect(160, 100, 160, 100, FB_YELLOW);

    // process_t *p = process_create_user(pong); scheduler_add(p); serial_print("Started pong\n"); 
    // process_t *hello = process_create_user(file_test); scheduler_add(hello); 

    // char elf_path[] = {'/', 'h','e','l','l','o','.','e','l','f', 0};
    // process_t *ep = process_create_elf(elf_path, 0);
    // if(ep) scheduler_add(ep); 
    char doom_path[] = {'/','d','o','o','m','.','e','l','f',0}; 
    process_t *doom = process_create_elf(doom_path, 1); 
    if(doom) scheduler_add(doom); 

    asm volatile("sti");
    scheduler_start();

    for(;;) asm volatile("sti\n\thlt");
}