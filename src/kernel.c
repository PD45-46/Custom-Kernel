// #include "kernel.h"
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


void kernel_main(void) { 
    vga_init(); 
    vga_print("Kernel Booting... \n"); 

    gdt_init(); 
    vga_print("[OK] GDT\n"); 

    idt_init(); 
    vga_print("[OK] IDT\n"); 

    pic_init(); 
    vga_print("[OK] PIC\n"); 

    timer_init(100);    /* 100Hz */
    vga_print("[OK] Timer\n");

    keyboard_init();
    vga_print("[OK] Keyboard\n");

    pmm_init();
    vga_print("[OK] Physical Memory Manager\n");

    vmm_init();
    vga_print("[OK] Virtual Memory Manager\n");

    heap_init();
    vga_print("[OK] Heap\n");

    scheduler_init(); 
    vga_print("[OK] Scheduler\n"); 

    process_t *a = process_create(process_A); 
    process_t *b = process_create(process_B); 
    process_t *c = process_create(process_C); 

    scheduler_add(a); 
    scheduler_add(b); 
    scheduler_add(c); 

    vga_print("Starting scheduler... \n"); 

    scheduler_start(); 
    // context_switch(a, b); 

    // /* enable interrupts */
    // asm volatile("sti");

    /* hang */
    for(;;) asm volatile("hlt"); 
}