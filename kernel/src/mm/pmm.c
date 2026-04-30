#include "pmm.h"
#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE 4096
#define KERNEL_HEAP_START 0xFFFF800000000000  
static uint64_t next_free = KERNEL_HEAP_START;
static uint64_t heap_end = KERNEL_HEAP_START + (64 * 1024 * 1024);

void pmm_init(struct limine_memmap_response* memmap) {
    (void)memmap;
}

void* pmm_alloc_page(void) {
    if (next_free + PAGE_SIZE > heap_end) return NULL;
    void* ptr = (void*)next_free;
    next_free += PAGE_SIZE;
    return ptr;
}

void pmm_free_page(void* ptr) {
    (void)ptr;
}

uint64_t pmm_get_free_page_count(void) {
    return (heap_end - next_free) / PAGE_SIZE;
}