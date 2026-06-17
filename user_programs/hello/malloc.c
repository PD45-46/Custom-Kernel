#include "malloc.h"
#include "../../src/user/user_lib.h"

#define ALIGN8(n)   (((uint64_t)(n) + 7ULL) & ~7ULL)
#define USED_MAGIC  0xA110CA7EU
#define FREE_MAGIC  0xFEEEFEEEU

typedef struct blk {
    uint64_t    size;   /* usable bytes after this header */
    uint32_t    magic;
    uint32_t    free;
    struct blk *next;
    struct blk *prev;
} blk_t;               /* 32 bytes */

static blk_t *heap_list;   /* .bss — zero-initialised by ELF loader */

static blk_t *blk_extend(uint64_t bytes) {
    blk_t *b = (blk_t *)u_sbrk((int64_t)(sizeof(blk_t) + bytes));
    if ((int64_t)(uint64_t)b == -1LL) return (blk_t *)0;
    b->size = bytes; b->magic = FREE_MAGIC; b->free = 1;
    b->next = b->prev = (blk_t *)0;
    return b;
}

void *malloc(uint64_t size) {
    if (!size) return (void *)0;
    size = ALIGN8(size);

    blk_t *b = heap_list, *prev = (blk_t *)0;
    while (b) {
        if (b->free && b->size >= size) {
            /* split if leftover fits a header + ≥8 bytes */
            if (b->size >= size + sizeof(blk_t) + 8ULL) {
                blk_t *tail  = (blk_t *)((uint8_t *)(b + 1) + size);
                tail->size   = b->size - size - sizeof(blk_t);
                tail->magic  = FREE_MAGIC; tail->free = 1;
                tail->next   = b->next;   tail->prev = b;
                if (b->next) b->next->prev = tail;
                b->next = tail; b->size = size;
            }
            b->free = 0; b->magic = USED_MAGIC;
            return (void *)(b + 1);
        }
        prev = b; b = b->next;
    }

    blk_t *nb = blk_extend(size);
    if (!nb) return (void *)0;
    nb->free = 0; nb->magic = USED_MAGIC;
    if (!heap_list) { heap_list = nb; }
    else            { nb->prev = prev; prev->next = nb; }
    return (void *)(nb + 1);
}

void free(void *ptr) {
    if (!ptr) return;
    blk_t *b = (blk_t *)ptr - 1;
    if (b->magic != USED_MAGIC) return;   /* double-free guard */
    b->free = 1; b->magic = FREE_MAGIC;

    /* coalesce right */
    if (b->next && b->next->free) {
        b->size += sizeof(blk_t) + b->next->size;
        b->next  = b->next->next;
        if (b->next) b->next->prev = b;
    }
    /* coalesce left */
    if (b->prev && b->prev->free) {
        b->prev->size += sizeof(blk_t) + b->size;
        b->prev->next  = b->next;
        if (b->next) b->next->prev = b->prev;
    }
}

void *realloc(void *ptr, uint64_t new_size) {
    if (!ptr)      return malloc(new_size);
    if (!new_size) { free(ptr); return (void *)0; }
    blk_t *b = (blk_t *)ptr - 1;
    if (b->size >= ALIGN8(new_size)) return ptr;
    void *fresh = malloc(new_size);
    if (!fresh) return (void *)0;
    uint8_t *s = (uint8_t *)ptr, *d = (uint8_t *)fresh;
    for (uint64_t i = 0; i < b->size; i++) d[i] = s[i];
    free(ptr); return fresh;
}

void *calloc(uint64_t nmemb, uint64_t size) {
    uint64_t total = nmemb * size;
    void *p = malloc(total);
    if (p) { uint8_t *b = (uint8_t *)p; for (uint64_t i=0;i<total;i++) b[i]=0; }
    return p;
}

/* I am not about to rewrite malloc again... */