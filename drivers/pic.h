#ifndef PIC_H
#define PIC_H
#include <stdint.h>
void pic_remap(uint8_t m, uint8_t s);
void pic_send_eoi(uint8_t irq);
void pic_unmask(uint8_t irq);
#endif