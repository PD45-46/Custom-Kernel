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

void user_process(void) { 
    char msg[] = "Hello World from ring 3\n"; 
    /* 
    Cant use VGA in ring 3 so gotta use syscalls 
    */
   /* Syscall for write with arg1 as string ptr, and arg2 as string len */
    asm volatile(
        "mov $0, %%rax\n"   /* SYS_WRITE = 0 */
        "mov %0, %%rdi\n"   /* arg1 = string pointer */
        "mov %1, %%rsi\n"   /* arg2 = length */
        "syscall\n"
        :
        : "r"((uint64_t)msg), "r"((uint64_t)(sizeof(msg)-1))
        : "rax", "rdi", "rsi"
    );
    /* yield after writing */
    asm volatile(
        "mov $2, %%rax\n"   /* SYS_YIELD = 2 */
        "syscall\n"
        : : : "rax"
    );
    for(;;); 
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

    process_t *a = process_create(process_A);
    process_t *b = process_create(process_B);
    process_t *c = process_create(process_C);
    // process_t *u = process_create_user(user_process); <- Issue... 
    scheduler_add(a);
    scheduler_add(b);
    scheduler_add(c);
    // scheduler_add(u); 

    vga_print("Starting scheduler...\n");
    asm volatile("sti");
    scheduler_start();

    for(;;) asm volatile("hlt");
}