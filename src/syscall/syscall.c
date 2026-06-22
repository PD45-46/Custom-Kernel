#include "syscall.h"
#include "../drivers/vga.h"
#include "../process/scheduler.h"
#include "../process/process.h" 
#include "../drivers/keyboard.h"
#include "../drivers/timer.h"
#include "../memory/vmm.h"
#include "../filesystem/ramdisk.h"
#include "../drivers/framebuffer.h"
#include <stdint.h> 
#include "../drivers/serial.h"


/*

*/
// #define USER_FB_VIRT 0x8000300000ULL
#define USER_FB_VIRT 0x9000000000ULL


// TODO LOOK AT NOTES.
/**
 * @brief Write a string to VGA and serial. 
 *        Serial something that will be developed in the future for testing... 
 * 
 * @param str_ptr Pointer to the string (user virtual address)
 * @param len Length of string 
 * @return int64_t Number of bits written or negative value for error. 
 * @note In a real kernel str_ptr must be validated that it points to the 
 *       the user space before doing anything. That will be skipped for now. 
 *       
 */
static int64_t sys_write(uint64_t str_ptr, uint64_t len) { 
    const char *str = (const char *)str_ptr;
    if(!str) return -1; 
    
    for(uint64_t i = 0; i < len; i++) { 
        vga_putchar(str[i]); 
        serial_putchar(str[i]); 
    } 
    return (int64_t)len; 

}

/**
 * @brief Terminate the current process. Marks teh current process as
 *        dead. The scheduler will skip it on the next tick.  
 * 
 * 
 * @param exit_code (Ignored for now)
 * @return int64_t 
 * @note Eventually the memory for the dead process will nee to be
 *       cleaned up --- for now it will just be marked as dead. 
 */
static int64_t sys_exit(uint64_t exit_code) { 
    (void)exit_code; 
    process_t *curr = scheduler_current(); 
    if(curr) { 
        curr->state = PROCESS_DEAD; 
    }
    scheduler_tick(); 
    for(;;) asm volatile("hlt"); 
    return 0;
}

/**
 * @brief Voluntarily give up the CPU. Calls scheduler_tick directly. 
 *        The current process goes back to ready state and the next one runs. 
 * 
 * @return int64_t 
 * @note Declare error states instead of just returning 0. 
 */
static int64_t sys_yield(void) { 
    scheduler_tick(); 
    /* TODO */
    return 0; 
}

/**
 * @brief 
 * 
 * @param ticks 
 * @return int64_t 0 on correct system procedure 
 */
static int64_t sys_sleep(uint64_t ticks) { 
    process_t *curr = scheduler_current(); 
    if(!curr) return -1; 
    curr->wake_tick = timer_ticks() + ticks; 
    curr->wait_reason = WAIT_SLEEP;
    curr->state = PROCESS_BLOCKED; 
    scheduler_tick();  
    return 0; 
}

/**
 * @brief 
 * 
 * @param buf_ptr 
 * @param len 
 * @return int64_t 
 */
static int64_t sys_read(uint64_t buf_ptr , uint64_t len) { 
    char *buf = (char *)buf_ptr; 
    if(!buf || len == 0) return -1; 
    for(uint64_t i = 0; i < len; i++) { 
        while(!keyboard_has_char()) { 
            process_t *curr = scheduler_current(); 
            curr->wait_reason = WAIT_KEY; 
            curr->state = PROCESS_BLOCKED; 
            scheduler_tick(); 
        }
        char c = keyboard_getchar(); 
        buf[i] = c; 
        if(c =='\n') return (int64_t)(i + 1); 
    }
    return (int64_t)len; 
} 

/**
 * @brief Gets the current process ID. 
 * 
 * @return uint64_t ID or -1 on error. 
 */
static uint64_t sys_getpid(void) { 
    process_t *curr = scheduler_current(); 
    if(!curr) return -1; 
    return (uint64_t)curr->pid; 
}

/**
 * @brief
 * @return 
 */
static int64_t sys_map_fb(void) { 
    process_t *curr = scheduler_current();
    if(!curr || !curr->page_table) return -1; 
    /* Map all 16 pages of the vga frame buffer window into user space */
    for(int i = 0; i < 16; i++) { 
        vmm_map_in(curr->page_table,
                   USER_FB_VIRT + i * 0x1000,
                   0xA0000 + i * 0x1000,
                   PTE_PRESENT | PTE_WRITABLE | PTE_USER);
            
    } 
    return (uint64_t)USER_FB_VIRT; 
}

/**
 * @brief
 * @return 
 */

static int64_t sys_getkey(void) { 
    return keyboard_has_char() ? keyboard_getchar() : 0; 
}


static int64_t sys_open (uint64_t path) { 
    return ramdisk_open((const char *)path); 
}
static int64_t sys_fread (uint64_t fd, uint64_t buf, uint64_t n) { 
    return ramdisk_read((int)fd, (void *)buf, n); 
}
static int64_t sys_fseek (uint64_t fd, uint64_t off, uint64_t w) { 
    return ramdisk_seek((int)fd, (int64_t)off, (int)w); 
}
static int64_t sys_fclose (uint64_t fd) { 
    ramdisk_close((int)fd); return 0; 
}
static int64_t sys_fsize (uint64_t fd) {
    return ramdisk_size((int)fd);
}

static int64_t sys_sbrk(int64_t increment) { 
    process_t *proc = scheduler_current(); 
    if(!proc || !proc->heap_start) return -1;
    if(increment == 0) return proc->heap_end; 
    
    uint64_t base = proc->heap_start; 
    uint64_t old_end = proc->heap_end; 
    uint64_t new_end = old_end + increment; 

    uint64_t old_pages = (old_end > base)
                       ? ((old_end - base + PAGE_SIZE - 1) / PAGE_SIZE) : 0;
    uint64_t new_pages = (new_end > base)
                       ? ((new_end - base + PAGE_SIZE - 1) / PAGE_SIZE) : 0;
    
    for(uint64_t i = old_pages; i < new_pages; i++) { 
        void *frame = pmm_alloc(); 
        if(!frame) return -1; 
        vmm_map_in(proc->page_table,
                   base + i * PAGE_SIZE,
                   (uint64_t)frame,
                   PTE_PRESENT | PTE_WRITABLE | PTE_USER); 
    }
    proc->heap_end = new_end;
    return old_end; 
}

static int64_t sys_gettime(void) { 
    return (int64_t)(timer_ticks() * 10); /* Hz to ms */
}

static int64_t sys_setpalette(uint64_t ptr) { 
    const int8_t *pal = (const uint8_t *)ptr; 
    for(int i = 0; i < 256; i++) { 
        fb_set_palette(i, pal[i*3], pal[i*3+1], pal[i*3+2]); 
    }
    return 0;
}

static int64_t sys_set_fs_base(uint64_t base) { 
    uint32_t lo = (uint32_t)base;
    uint32_t hi = (uint32_t)(base >> 32);
    asm volatile("wrmsr" :: "c"(0xC0000100U), "a"(lo), "d"(hi));
    return 0;
}


/**
 * @brief 
 * 
 * @param num 
 * @param arg1 
 * @param arg2 
 * @param arg3 
 * @return int64_t 
 */
int64_t syscall_dispatch(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3) { 
    /* Serials... */
    switch(num) { 
        case SYS_WRITE:       return sys_write(arg1, arg2); 
        case SYS_EXIT:        return sys_exit(arg1); 
        case SYS_YIELD:       return sys_yield(); 
        case SYSGETPID:       return sys_getpid(); 
        case SYS_SLEEP:       return sys_sleep(arg1); 
        case SYS_READ:        return sys_read(arg1, arg2); 
        case SYS_MAP_FB:      return sys_map_fb();
        case SYS_GETKEY:      return sys_getkey();
        case SYS_OPEN:        return sys_open(arg1); 
        case SYS_FREAD:       return sys_fread(arg1, arg2, arg3);
        case SYS_FSEEK:       return sys_fseek(arg1, arg2, arg3);
        case SYS_FCLOSE:      return sys_fclose(arg1);
        case SYS_FSIZE:       return sys_fsize(arg1);
        case SYS_SBRK:        return sys_sbrk(arg1); 
        case SYS_GETTIME:     return sys_gettime(); 
        case SYS_SETPALETTE:  return sys_setpalette(arg1); 
        case SYS_SET_FS_BASE: return sys_set_fs_base(arg1); 
        default: 
            /* Serials... */
            return -1; 
    }


}