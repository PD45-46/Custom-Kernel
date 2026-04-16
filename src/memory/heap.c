#include "heap.h"
#include "vmm.h"
#include "pmm.h"
#include "../drivers/vga.h"
#include <stdint.h>
#include <stddef.h>

/*
The kernel heap lives at a virtual fixed address. 
Map physical frames here on demand.  
*/
#define HEAP_START 0xC0000000ULL
#define HEAP_PAGES 1024

/*
I have already created my own malloc before, so I won't going into 
a great deal of detail about this. Just know that memory layout is like this: 

|      Block 1     ||      Block 2     |     
[header][...data...][header][...data...]
        ^-- returned pointer 

*/
typedef struct block_header  {
    size_t size;                  // size of data region (excluding header)
    int free;                     // 1 = free, 0 = used 
    struct block_header *next;    // next block in linked list 
} block_header_t; 

static block_header_t *heap_head = NULL; 

/**
 * @brief Maps physical frames for the heap and creates one large 
 *        free block covering the entire heap region. 
 * 
 */
void heap_init() { 
    for(size_t i = 0; i < HEAP_PAGES; i++) { 
        void *frame = pmm_alloc(); 
        if(!frame) { 
            vga_print("Invalid frame\n"); 
            for(;;) asm volatile("hlt"); 
        }
        // TODO Make sure flags and etc are accessible 
        vmm_map(HEAP_START + i * PAGE_SIZE, (uint64_t)frame, 0x03 /* Present + Writable*/);
    }
    heap_head = (block_header_t *)HEAP_START;
    heap_head->size = (HEAP_PAGES * PAGE_SIZE) - sizeof(block_header_t); 
    heap_head->free = 1; 
    heap_head->next = NULL; 
}

/**
 * @brief Find a free block big enough, split if we need to, 
 *        mark as used, return pointer of the data region. This 
 *        is using the First-Fit strategy --- take the first 
 *        available block that is big enough. 
 * 
 * @param size Number of bytes to allocate
 * @return void* Pointer to 
 */
void *kmalloc(size_t size) { 
    if(!size) return NULL; 

    size = (size + 7) & ~7ULL; // align size to 8 bytes 

    block_header_t *curr = heap_head; 
    while(curr) { 
        if(curr->free && curr->size >= size) { 
            // split block if there is enough room for a new header 
            if(curr->size > size + sizeof(block_header_t) + 8) { 
                block_header_t *split = (block_header_t*)(
                    (uint8_t *)curr + sizeof(block_header_t) + size);  
                split->size = curr->size - size - sizeof(block_header_t); 
                split->free = 1; 
                split->next = curr->next; 
                curr->next = split; 
                curr->size = size; 
            }
            curr->free = 0; 
            return (void *)((uint8_t *)curr + sizeof(block_header_t)); 
        }
        curr = curr->next; 
    }
    return NULL; 
}


/**
 * @brief Works the exact same as calloc(). Allocate and zero memory. 
 * 
 * @param n 
 * @param size 
 * @return void* 
 */
void *kcalloc(size_t n, size_t size) { 
    void *ptr = kmalloc(n * size); 
    if(ptr) { 
        uint8_t *p = ptr; 
        for(size_t i = 0; i < n * size; i++) { 
            p[i] = 0; 
        }
    }
    return ptr; 
}

/**
 * @brief Marks a block as free and merges adjacent free blocks. 
 *        It uses coalescing to avoid memory fragmentation --- merge 
 *        right hand side free block and current one into one larger block.  
 * 
 *        Note that memory fragmentation is a point where too many
 *        blocks are split (and not coalesced) during allocation leaving 
 *        tiny 'fragments' of memory with large headers. It is easy to 
 *        imagine why this becomes an issue in systems. 
 * 
 * @param ptr 
 */
void kfree(void *ptr) { 
    if(!ptr) return; 

    block_header_t *header = (block_header_t *)( 
        (uint8_t *)ptr - sizeof(block_header_t)); 
    header->free = 1; 

    // coalesce 
    block_header_t *curr = heap_head; 
    while(curr && curr->next) { 
        if(curr->free && curr->next->free) { 
            curr->size += sizeof(block_header_t) + curr->next->size; 
        } else { 
            curr = curr->next; 
        }
    }
}


