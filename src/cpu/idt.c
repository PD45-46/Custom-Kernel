#include "idt.h"
#include "../drivers/vga.h"
#include <stdint.h> 
#include <stddef.h> 

// TODO CLEANUP COMMENTS AND INIT ISR ARRAY WITH HANDLERS 

/*
Intel and AMD processors are designed to support a max of 256 interrupts. 
0 - 31 (32): Exceptions 
32 - 47 (16): IRQs (Interrupt Requests) 
48 - 255 (208): User defined  
*/
static idt_entry_t idt[256] __attribute__((aligned(16))); 
static idt_ptr_t idt_ptr; 

/* 
Array of C-level handlers, one per interrupt. 
Initially all NULL 
The common handler just prints and halts. 
*/
static void (*isr_handlers[256])(registers_t *) = { NULL }; 

/* 
Defined in isr.arm 
Table of assembly stub addresses. 
*/
extern uint64_t isr_stub_table[]; 

/** 
 * @brief Installs one handler into the IDT. 
 * @param n IDT slot number: 0 - 255
 * @param handler Memory address of the assembly code for the CPU to run 
 * @param type_attr Permissions
 * @note type_attr bit mapping: 
 *  - Gate Bits (0 - 3). Defines the kind of gate. 0xE (0b1110) is the interrupt gate. 
 *  - Storage Segment (4). Always set to 0 for interrupt and trap gates. 
 *  - Descriptor Privilege (6 - 5) Level. Defines ring level. 0b00 for ring 0 (kernel). 
 *    0b11 for ring 3 (user). 
 *  - Present (7). Must be 1 for entry to be valid, else SPU triggers "Segment not present" 
 *    exception.   
 * 
 *   0x8E = present, ring 0, 64-bit interrupt gate
 *   0xEE = present, ring 3, 64-bit interrupt gate (for syscalls)
 */
static void idt_set_entry(uint8_t n, uint64_t handler, uint8_t type_attr) { 
    idt[n].offset_low = handler & 0xFFFF; 
    idt[n].offset_mid = (handler >> 16) & 0xFFFF; 
    idt[n].offset_high = (handler >> 32) & 0xFFFFFFFF;
    idt[n].segment_selector = 0x08; // kernel 
    idt[n].ist = 0; 
    idt[n].type_attr = type_attr; 
    idt[n].zero = 0; 
}

/** 
 * @brief Called from every exception stub in isr.asm. Dispatches to the registered
 *        C handler or prints out an error. 
 * @param regs Pointer to the exact state of the CPU at the moment of the interrupt
 */
void isr_common_handler(registers_t *regs) { 
    if(isr_handlers[regs->int_no]) { 
        isr_handlers[regs->int_no](regs); // goto addr and exec func using regs as param
    } else { 
        vga_print("EXCEPTION: int=0x"); 

        uint64_t n = regs->int_no; 
        for (int i = 60; i >= 0; i -= 4) {
            uint8_t nibble = (n >> i) & 0xF;
            char c = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
            if (c != '0' || i == 0) vga_putchar(c); 
        }
        vga_print("  err=");
        vga_print_hex(regs->err_code); 
        vga_print("\n");
        for(;;) asm volatile("hlt");
    }
}

/** 
 * @brief Called from every IRQ stub in irq.asm. Same dispatch logic as 
 *        isr_common_handler but for hardware interrupts (32 - 47). 
 * 
 */
void irq_common_handler(registers_t *regs) { 
    if(isr_handlers[regs->int_no]) 
        isr_handlers[regs->int_no](regs);  
}

/**
 * @brief Places the handler function in the associated location 
 *        in the Interrupt Service Routine array. 
 * 
 * @param n 
 * @param handler 
 */
void idt_register_handler(uint8_t n, void (*handler)(registers_t *)) {
    isr_handlers[n] = handler;
}


static void page_fault_handler(registers_t *regs) {
    (void)regs;  
    uint64_t fault_addr; 
    asm volatile("mov %%cr2, %0" : "=r"(fault_addr)); 
    vga_print("PAGE FAULT at addr="); 
    vga_print_hex(fault_addr); 
    vga_print(" err="); 
    vga_print_hex(regs->err_code); 
    vga_print("\n"); 
    /*
     * Error code bits:
     *   bit 0 = 1: protection violation (page present, access denied)
     *   bit 0 = 0: page not present
     *   bit 1 = 1: write access
     *   bit 2 = 1: user mode access
     *   bit 4 = 1: instruction fetch
     */
    for(;;) asm volatile("hlt"); 
}   

static void ud_handler(registers_t *regs) { 

    vga_print("\n --- DEBUG DUMP --- \n"); 
    uint64_t *stack = (uint64_t*)regs->rsp; 
    for(int i = 0; i < 8; i++) { 
        vga_print("Stack["); vga_print_hex(i); vga_print("]: ");
        vga_print_hex(stack[i]);
        vga_print("\n");
    }

    vga_print("\n[EXCEPTION] Invalid Opcode (#UD)!\n");
    vga_print("RIP="); 
    vga_print_hex(regs->rip);
    vga_print("\nRAX="); 
    vga_print_hex(regs->rax); 
    vga_print("\nCS="); 
    vga_print_hex(regs->cs); 
    vga_print("\n"); 
    
    for(;;) asm volatile("hlt");
}



void idt_init(void) { 
    idt_ptr.limit = sizeof(idt) - 1; 
    idt_ptr.base = (uintptr_t)&idt; 

    for(int i = 0; i < 48; i++) {
        idt_set_entry(i, isr_stub_table[i], 0x8E); 
    }
    asm volatile("lidt %0" : : "m"(idt_ptr)); 
    idt_register_handler(14, page_fault_handler); 
    idt_register_handler(6, ud_handler); 
}



