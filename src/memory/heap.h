#pragma once
#include <stddef.h> 

/* 
Dynamic allocation inside the kernel 
*/

void heap_init(void); 
void *kmalloc(size_t size); 
void *kcalloc(size_t n, size_t size); 
void kfree(void *ptr); 