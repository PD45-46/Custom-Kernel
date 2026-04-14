#pragma once
#include <stddef.h> 

void heap_init(); 
void *kmalloc(size_t size); 
void *kcalloc(size_t n, size_t size); 
void kfree(void *ptr); 