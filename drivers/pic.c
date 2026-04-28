#include "pic.h"
#include "io.h"
#define P1C 0x20
#define P1D 0x21
#define P2C 0xA0
#define P2D 0xA1

void pic_remap(uint8_t m, uint8_t s) {
    uint8_t a1=inb(P1D), a2=inb(P2D);
    outb(P1C,0x11); outb(P2C,0x11);
    outb(P1D,m); outb(P2D,s);
    outb(P1D,4); outb(P2D,2);
    outb(P1D,1); outb(P2D,1);
    outb(P1D,a1); outb(P2D,a2);
    outb(P1D,0xFC); // mascara tudo menos IRQ0,1
    outb(P2D,0xFF);
}
void pic_send_eoi(uint8_t irq) { if(irq>=8) outb(P2C,0x20); outb(P1C,0x20); }
void pic_unmask(uint8_t irq) {
    uint16_t p; uint8_t v;
    if(irq<8){p=P1D;}else{p=P2D;irq-=8;}
    v=inb(p)&~(1<<irq); outb(p,v);
}