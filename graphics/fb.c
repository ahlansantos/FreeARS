#include "fb.h"
#include "font.h"
framebuffer_t fb={0};
void fb_init(uint32_t magic, uint32_t mbi) {
    if(magic!=0x2BADB002){fb.available=0;return;}
    uint32_t*m=(uint32_t*)mbi;
    if(!(m[0]&(1<<11))){fb.available=0;return;}
    fb.address=(uint32_t*)(uint64_t)m[22];
    fb.pitch=m[24]; fb.width=m[25]; fb.height=m[26]; fb.bpp=m[27];
    fb.available=1;
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