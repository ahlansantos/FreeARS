#pragma once
#include <stdint.h>
#include <stddef.h>
#include <limine.h>

void pmm_init(struct limine_memmap_response* memmap);
void* pmm_alloc_page(void);
void pmm_free_page(void* ptr);
uint64_t pmm_get_free_page_count(void);