#pragma once
#include <stddef.h>

#define PAGE_SIZE 4096

void *kmalloc(size_t size);
void  kfree(void *ptr);