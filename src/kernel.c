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
        asm volatile("hlt"); 
    }
}

void process_B(void) { 
    while(1) { 
        vga_print("B "); 
        asm volatile("hlt"); 
    }

}

void process_C(void) {
    int arr[6] = {1, 2, 3, 4, 5, 6};   
    while(1) { 
        uint64_t t = timer_ticks(); 
        vga_print_int(arr[t % 6] + 1); 
        asm volatile("hlt"); 
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
    msg[0] = 'R'; 
    msg[1] = '3'; 
    msg[2] = '!'; 
    msg[3] = '\n'; 
    msg[4] = '\0'; 
    while(1) { 
        uint64_t ptr = (uint64_t)msg; 
        uint64_t len = sizeof(msg) - 1; 

        /* SYS_WRITE */
        asm volatile( 
            "mov $0, %%rax\n"
            "syscall\n"
            : : "D"(ptr), "S"(len)
            : "rax", "rcx", "r11", "memory"
        ); 
        /* SYS_YIELD */
        asm volatile(
            "mov $2, %%rax\n"
            "syscall\n"
            : : : "rax", "rcx", "r11"
        ); 
        asm volatile("hlt"); 
   }
}


void kernel_main(void) { 
    serial_init();
    vga_init();
    vga_print("Kernel Booting...\n");

    gdt_init();      vga_print("[OK] GDT\n");
    idt_init();      vga_print("[OK] IDT\n");
    pic_init();      vga_print("[OK] PIC\n");
    
    keyboard_init(); vga_print("[OK] Keyboard\n");
    pmm_init();      vga_print("[OK] PMM\n");
    vmm_init();      vga_print("[OK] VMM\n");
    heap_init();     vga_print("[OK] Heap\n");

    timer_init(100); vga_print("[OK] Timer\n");
    scheduler_init();  vga_print("[OK] Scheduler\n");
    syscall_init();    vga_print("[OK] Syscall\n");

    
    #ifdef RUN_TESTS
        vga_print("[RUNNING TESTS]\n");
        asm volatile("cli"); 
        run_all_tests();
        asm volatile("sti"); 
        vga_print("[TESTS DONE]\n");
    #endif

    vga_print("Kernel PML4 Index: ");
    vga_print_int(PML4_INDEX(0x100C00)); // Should be 0
    vga_print("\n"); 

    // process_t *a = process_create(process_A); scheduler_add(a);
    // process_t *b = process_create(process_B); scheduler_add(b);
    // process_t *c = process_create(process_C); scheduler_add(c); 
    process_t *u = process_create_user(user_process); scheduler_add(u); 
    

    vga_print("Starting scheduler...\n");
    asm volatile("sti");
    scheduler_start();

    for(;;) asm volatile("hlt");
}