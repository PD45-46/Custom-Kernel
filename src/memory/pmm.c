#include "pmm.h"
#include "../drivers/vga.h"
#include <stdint.h> 
#include <stddef.h> 

/*
Assume 256MB of physical RAM. Each bit in the bitmap 
represents one 4KB page frame. 
Total frames = 256MB / 4KB = 65536 frames.
Bitmap size  = 65536 / 8 = 8192 bytes. 
*/

#define TOTAL_MEMORY (256 * 1024 * 1024) 
#define TOTAL_FRAMES (TOTAL_MEMORY / PAGE_SIZE)
#define BITMAP_SIZE (TOTAL_FRAMES / 8) 

static uint8_t bitmap[BITMAP_SIZE]; 
static size_t free_frame_count = 0; 

/* 
 * bitmap_set/clear/test - manipulate individual bits
 * frame_index / 8 gives the byte, frame_index % 8 gives the bit
 */
static void bitmap_set(uint32_t frame) {
    bitmap[frame / 8] |= (1 << (frame % 8));
}

static void bitmap_clear(uint32_t frame) {
    bitmap[frame / 8] &= ~(1 << (frame % 8));
}

static int bitmap_test(uint32_t frame) {
    return bitmap[frame / 8] & (1 << (frame % 8));
}

/**
 * @brief Marks all the frames used initially, then frees the usable ones. 
 *        We reserve the fist 1MB (frames 0 - 255) because the region contains 
 *        BIOS data, VGA buffer, and the kernel code. 
 * 
 */
void pmm_init() { 
    // mark everything as used 
    for(size_t i = 0; i < BITMAP_SIZE; i++) { 
        bitmap[i] = 0xFF; 
    }

    // Free frames from 1MB to end of usable RAM  
    uint32_t start_frame = (1 * 1024 * 1024) / PAGE_SIZE;
    uint32_t end_frame = TOTAL_FRAMES; 
    
    for(uint32_t i = start_frame; i < end_frame; i++) { 
        bitmap_clear(i); 
        free_frame_count++; 
    }
}

/**
 * @brief Finds the first free frame, marks it as used, and returns the 
 *        physical address to it. Returns NULL if no frames are available. 
 * 
 * @return void* NULL or address to allocated memory slot. 
 * @note Using a bitmap will cost O(n) scan times (which is fine for now), but 
 *       in the real world we would use free lists for O(1) time. 
 */
void *pmm_alloc() { 
    for(uint32_t i = 0; i < TOTAL_FRAMES; i++) { 
        if(!bitmap_test(i)) { 
            bitmap_set(i); 
            free_frame_count--; 
            return (void *)(uint32_t)(i * PAGE_SIZE); 
        }
    }
    return NULL; 
}

/**
 * @brief Marks a frame as free again. 
 * 
 * @param frame Address of allocated memory to free. 
 */
void pmm_free(void *frame) { 
    uint32_t idx = (uint32_t)frame / PAGE_SIZE; 
    bitmap_clear(idx); 
    free_frame_count++; 
}

/**
 * @brief Simple getter for the number of free frames. 
 * 
 * @return size_t Number of free frames. 
 */
size_t pmm_free_frames() { 
    return free_frame_count; 
}