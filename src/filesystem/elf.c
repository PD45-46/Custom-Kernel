#include "elf.h"
#include "ramdisk.h"
#include "../memory/vmm.h"
#include "../memory/pmm.h"
#include "../memory/heap.h"
#include "../process/process.h"
#include "../drivers/serial.h"
#include <stdint.h>

/**
 * @brief
 * @param 
 * @return 
 */
process_t *process_create_elf(const char *path, int use_linux_abi) { 
    int fd = ramdisk_open(path); 
    if(fd < 0) { 
        serial_print("ELF: file not found\n"); 
        return NULL; 
    }

    Elf64_Ehdr ehdr; 
    ramdisk_read(fd, &ehdr, sizeof(ehdr)); 

    if (ehdr.e_ident[0] != 0x7F || ehdr.e_ident[1] != 'E' ||
        ehdr.e_ident[2] != 'L'  || ehdr.e_ident[3] != 'F' ||
        ehdr.e_ident[4] != 2    || ehdr.e_machine   != EM_X86_64) {
        serial_print("ELF: bad header\n");
        ramdisk_close(fd);
        return NULL;
    }

    process_t *proc = process_alloc(); 
    if(!proc) { 
        ramdisk_close(fd); 
        return NULL; 
    }

    proc->page_table = vmm_create_address_space(); 
    if(!proc->page_table) { 
        process_destroy(proc); 
        ramdisk_close(fd); 
        return NULL; 
    }

    for(int i = 0; i < USER_STACK_PAGES; i++) { 
        void *frame = pmm_alloc();
        if(!frame) { 
            ramdisk_close(fd); 
            return NULL; 
        }
        uint64_t virt = USER_STACK_VIRT - ((USER_STACK_PAGES - i) * PAGE_SIZE);
        vmm_map_in(proc->page_table, virt, (uint64_t)frame,
                   PTE_PRESENT | PTE_WRITABLE | PTE_USER);
    }
    proc->user_stack = USER_STACK_VIRT - 8; 
    proc->use_linux_abi = (uint8_t)use_linux_abi; 

    uint64_t saved_cr3; 
    asm volatile("movq %%cr3, %0" : "=r"(saved_cr3)); 

    uint64_t last_vend = 0; 

    for(int i = 0; i < ehdr.e_phnum; i++) { 
        Elf64_Phdr phdr;
        ramdisk_seek(fd, (int64_t)(ehdr.e_phoff + i * sizeof(Elf64_Phdr)), SEEK_SET);
        ramdisk_read(fd, &phdr, sizeof(phdr));

        if(phdr.p_type != PT_LOAD || phdr.p_memsz == 0) continue;

        uint64_t vstart = phdr.p_vaddr & ~0xFFFULL; /* recall the fun for this... */
        uint64_t vend = (phdr.p_vaddr + phdr.p_memsz + 0xFFF) & ~0xFFFULL;
        if(vend > last_vend) last_vend = vend; 

        uint32_t flags = PTE_PRESENT | PTE_USER | PTE_WRITABLE; 

        for(uint64_t v = vstart; v < vend; v += PAGE_SIZE) {
            uint8_t *frame = (uint8_t *)pmm_alloc();
            if (!frame) { ramdisk_close(fd); return NULL; }
            vmm_map_in(proc->page_table, v, (uint64_t)frame, flags);
        }

        vmm_switch_address_space(proc->page_table); 
        uint8_t *seg = (uint8_t *)phdr.p_vaddr; 
        for(uint64_t z = 0; z < phdr.p_memsz; z++) { 
            seg[z] = 0; 
        }
        ramdisk_seek(fd, (int64_t)phdr.p_offset, SEEK_SET);
        ramdisk_read(fd, (void *)phdr.p_vaddr, phdr.p_filesz);
        vmm_switch_address_space(saved_cr3);
    
    }
    ramdisk_close(fd); 

    proc->heap_start = last_vend ? last_vend : USER_HEAP_BASE; 
    proc->heap_end = proc->heap_start; 

    extern void process_user_trampoline_fn(void); 
    uint64_t *sp = (uint64_t *)proc->kernel_stack; 
    sp--; *sp = proc->user_stack;
    sp--; *sp = ehdr.e_entry;
    sp--; *sp = (uint64_t)process_user_trampoline_fn;
    proc->context.rsp = (uint64_t)sp;

    serial_print("ELF loaded: entry=");
    serial_print_hex(ehdr.e_entry);
    serial_print("\n");
    return proc;
}

