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

/* 
Task State Segment 
Ony need the rsp0 register to find the 
kernel stack pointer. 
*/
typedef struct __attribute__((packed)) {
    uint32_t reserved0;
    uint64_t rsp0;        /* kernel stack pointer for ring 0 */
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];      /* interrupt stack table — unused for now */
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopb_offset;
} tss_t; 

/*
 * TSS GDT descriptor is 16 bytes, not 8.
 * We store it in two consecutive GDT slots (indices 5 and 6).
 */
typedef struct __attribute__((packed)) {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t type;        /* 0x89 = present, ring 0, 64-bit TSS available */
    uint8_t limit_high_flags;
    uint8_t base_high;
    uint32_t base_upper; /* upper 32 bits of base address */
    uint32_t reserved;
} tss_descriptor_t;

void gdt_init(void); 
void syscall_init(void); 
void tss_set_kernel_stack(uint64_t rsp0); 