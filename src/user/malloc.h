#pragma once
#include <stdint.h>

void *malloc(uint64_t size);
void  free(void *ptr);
void *realloc(void *ptr, uint64_t new_size);
void *calloc(uint64_t nmemb, uint64_t size);