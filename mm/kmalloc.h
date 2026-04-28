#ifndef KMALLOC_H
#define KMALLOC_H
#include <stdint.h>
void kmalloc_init();
void *kmalloc(uint32_t size);
void kfree(void *ptr);
#endif