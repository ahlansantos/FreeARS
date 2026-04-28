#include "timer.h"
#include "io.h"
#include "pic.h"
static uint32_t ticks=0;
void timer_init(uint32_t f) {
    uint32_t d=1193180/f;
    outb(0x43,0x36); outb(0x40,d&0xFF); outb(0x40,(d>>8)&0xFF);
    pic_unmask(0);
}
void timer_handler() { ticks++; pic_send_eoi(0); }
uint32_t timer_get_ticks() { return ticks; }
void sleep_ms(uint32_t ms) {
    uint32_t t=ticks+(ms*100)/1000;
    if(t==ticks)t++;
    while(ticks<t) __asm__ volatile("hlt");
}