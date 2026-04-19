#pragma once
#include <stdint.h> 

/* Model Specific Addresses for toggling CPU features */
#define MSR_STAR   0xC0000081 /* Which privilege levels / segments to switch between*/
#define MSR_LSTAR  0xC0000082 /* Jump here when syscall happens */
#define MSR_SFMASK 0xC0000084 /* Clean CPU flags on syscall entry */

/* TO HELP ENABLE SYSCALL INSTRUCTION */
#define MSR_EFER 0xC0000080
#define EFER_SCE (1 << 0)

typedef struct __attribute__((packed)) { 
    uint16_t limit_low; 
    uint16_t base_low; 
    uint8_t base_mid; 
    uint8_t access; 
    uint8_t granularity; 
    uint8_t base_high; 
} gdt_entry_t; 

typedef struct __attribute__((packed)) { 
    uint16_t limit; 
    uint64_t base; 
} gdt_ptr_t; 

void gdt_init(void); 
void syscall_init(void); 