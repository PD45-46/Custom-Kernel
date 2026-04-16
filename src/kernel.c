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

void kernel_main() { 
    vga_init(); 
    vga_print("Kernel Booting... \n"); 

    gdt_init(); 
    vga_print("[OK] GDT\n"); 

    idt_init(); 
    vga_print("[OK] IDT\n"); 

    // pic_init(); 
    // vga_print("[OK] PIC\n"); 

    // timer_init(100);    /* 100Hz */
    // vga_print("[OK] Timer\n");

    // keyboard_init();
    // vga_print("[OK] Keyboard\n");

    // pmm_init();
    // vga_print("[OK] Physical memory manager\n");

    // vmm_init();
    // vga_print("[OK] Virtual memory manager\n");

    // heap_init();
    // vga_print("[OK] Heap\n");

    // vga_print("Kernel ready.\n");

    // /* enable interrupts */
    // asm volatile("sti");

    /* hang */
    for(;;) asm volatile("hlt"); 
}