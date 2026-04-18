#pragma once
#include <stdint.h> 

/* 
Programmable Interrupt Controller, the method to handling hardware 
interrupts. If not configured, hardware interrupts conflict with CPU 
exception numbers. 
*/

#define PIC1_CMD 0x20 
#define PIC1_DATA 0x21 
#define PIC2_CMD 0xA0 
#define PIC2_DATA 0xA1

void pic_init(void); 
void pic_send_eoi(uint8_t irq); // end of interrupt signal 
void pic_mask(uint8_t irq); // disable specific irq
void pic_unmask(uint8_t irq); // enable specific irq