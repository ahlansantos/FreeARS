#include <stdint.h>
#include "../graphics/fb.h"
typedef struct{uint64_t r15,r14,r13,r12,r11,r10,r9,r8,rbp,rdi,rsi,rdx,rcx,rbx,rax;}regs_t;

void exception_handler(regs_t*r){
    uint64_t*st=(uint64_t*)(r+1);uint64_t num=st[0];
    if(!fb.available){
        volatile unsigned short*v=(unsigned short*)0xB8000;
        for(int i=0;i<80*25;i++)v[i]=0x4F20;
        char*m="EXCEPTION! Halted.";
        for(int i=0;m[i];i++)v[i]=m[i]|0x4F00;
        __asm__ volatile("cli");for(;;)__asm__ volatile("hlt");
    }
    uint32_t*fb_ptr=fb.address;
    uint32_t total=fb.height*(fb.pitch/4);
    for(uint32_t i=0;i<total;i++)fb_ptr[i]=0x00AA0000;
    uint32_t cx=fb.width/2-60,cy=fb.height/2-20,white=0xFFFFFF;
    for(uint32_t dy=0;dy<40;dy++)for(uint32_t dx=0;dx<120;dx++)
        if(dy<2||dy>=38||dx<2||dx>=118)
            *(uint32_t*)((uint8_t*)fb.address+(cy+dy)*fb.pitch+(cx+dx)*4)=white;
    char*msg="EXCEPTION #";
    for(int i=0;msg[i];i++)fb_draw_char(cx+8+i*8,cy+12,msg[i],white,0xAA0000);
    char ns[3];ns[0]='0'+(num/10);ns[1]='0'+(num%10);ns[2]=0;
    fb_draw_char(cx+8+11*8,cy+12,ns[0],white,0xAA0000);
    fb_draw_char(cx+8+12*8,cy+12,ns[1],white,0xAA0000);
    __asm__ volatile("cli");for(;;)__asm__ volatile("hlt");
}