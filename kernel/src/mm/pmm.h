#pragma once
#include <stdint.h>
#include <stddef.h>
#include <limine.h>

#define PAGE_SIZE 4096

void     pmm_init(struct limine_memmap_response *memmap, uint64_t hhdm_offset);
void    *pmm_alloc_page(void);   // retorna endereço FÍSICO
void     pmm_free_page(void *phys);
uint64_t pmm_get_free_page_count(void);
uint64_t pmm_phys_to_virt(uint64_t phys); // converte físico → virtual (HHDM)