#include "fb.h"
#include "font.h"
framebuffer_t fb={0};

void fb_init(uint64_t magic, uint64_t mbi) {
    if (magic != 0x36d76289) {
        fb.available = 0;
        return;
    }

    uint8_t *tags = (uint8_t*)mbi;
    uint32_t total_size = *(uint32_t*)tags;
    uint8_t *current = tags + 8;
    uint8_t *end = tags + total_size;

    while (current < end) {
        uint32_t type = *(uint32_t*)(current);
        uint32_t size = *(uint32_t*)(current + 4);

        if (type == 0) break;
        if (type == 8) {
            uint64_t fb_addr = *(uint64_t*)(current + 8);
            uint32_t pitch = *(uint32_t*)(current + 16);
            uint32_t width = *(uint32_t*)(current + 20);
            uint32_t height = *(uint32_t*)(current + 24);
            uint32_t bpp = *(uint32_t*)(current + 28);

            if (fb_addr && width && height) {
                fb.address = (uint32_t*)fb_addr;
                fb.pitch = pitch;
                fb.width = width;
                fb.height = height;
                fb.bpp = bpp;
                fb.available = 1;
                return;
            }
        }
        current += ((size + 7) / 8) * 8;
    }

    fb.available = 0;
}

void fb_put_pixel(uint32_t x,uint32_t y,uint32_t c){
    if(!fb.available||x>=fb.width||y>=fb.height)return;
    uint32_t*p=(uint32_t*)((uint8_t*)fb.address+y*fb.pitch+x*4);*p=c;
}
void fb_fill_rect(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint32_t c){
    for(uint32_t iy=0;iy<h;iy++)for(uint32_t ix=0;ix<w;ix++)fb_put_pixel(x+ix,y+iy,c);
}
void fb_clear(uint32_t c){fb_fill_rect(0,0,fb.width,fb.height,c);}
void fb_draw_char(uint32_t x,uint32_t y,char ch,uint32_t fg,uint32_t bg){
    const uint8_t*g=font[(unsigned char)ch];
    for(int r=0;r<16;r++)for(int c=0;c<8;c++)
        fb_put_pixel(x+c,y+r,(g[r]&(1<<(7-c)))?fg:bg);
}
void fb_draw_text(uint32_t x,uint32_t y,const char*t,uint32_t fg,uint32_t bg){
    for(int i=0;t[i];i++)fb_draw_char(x+i*8,y,t[i],fg,bg);
}