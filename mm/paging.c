#include "paging.h"
#define P 1
#define W 2
#define S 0x80

static uint64_t pml4[512] __attribute__((aligned(4096)));
static uint64_t pdpt[512] __attribute__((aligned(4096)));
static uint64_t pd[4][512] __attribute__((aligned(4096)));

void paging_init() {
    for (int i = 0; i < 4; i++) {
        pdpt[i] = (uint64_t)&pd[i] | P | W;
        for (int j = 0; j < 512; j++) {
            pd[i][j] = (uint64_t)(i * 0x40000000 + j * 0x200000) | P | W | S;
        }
    }
    pml4[0] = (uint64_t)&pdpt | P | W;
    __asm__ volatile("mov %0, %%cr3" :: "r"(&pml4));
}