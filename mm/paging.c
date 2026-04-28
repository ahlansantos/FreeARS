#include "paging.h"
#define P 1
#define W 2
#define S 0x80
static uint32_t pd[1024]__attribute__((aligned(4096)));
void paging_init(){
    pd[0]=0x000000|P|W|S;
    pd[1]=0x400000|P|W|S;
    pd[2]=0x800000|P|W|S;
    pd[3]=0xC00000|P|W|S;
    uint32_t fb_pde=0xFD000000>>22;
    pd[fb_pde]=0xFD000000|P|W|S;
    for(int i=4;i<1024;i++)if(i!=fb_pde)pd[i]=0;
    __asm__ volatile("mov %0,%%cr3"::"r"(pd));
    __asm__ volatile("mov %%cr4,%%eax; or $0x10,%%eax; mov %%eax,%%cr4":::"eax");
    __asm__ volatile("mov %%cr0,%%eax; or $0x80000000,%%eax; mov %%eax,%%cr0":::"eax");
}