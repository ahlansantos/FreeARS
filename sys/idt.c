#include "idt.h"
static idt_entry_t idt[256]; static idt_ptr_t idt_ptr;
extern void isr0(),isr6(),isr8(),isr13(),isr14(),irq0();
static void set(uint8_t n, uint64_t h, uint16_t s, uint8_t f) {
    idt[n].base_low = h & 0xFFFF;
    idt[n].base_mid = (h >> 16) & 0xFFFF;
    idt[n].base_high = (h >> 32) & 0xFFFFFFFF;
    idt[n].selector = s;
    idt[n].ist = 0;
    idt[n].flags = f;
    idt[n].reserved = 0;
}
void idt_init() {
    for (int i = 0; i < 256; i++) set(i, 0, 0, 0);
    set(0, (uint64_t)isr0, 0x08, 0x8E);
    set(6, (uint64_t)isr6, 0x08, 0x8E);
    set(8, (uint64_t)isr8, 0x08, 0x8E);
    set(13, (uint64_t)isr13, 0x08, 0x8E);
    set(14, (uint64_t)isr14, 0x08, 0x8E);
    set(0x20, (uint64_t)irq0, 0x08, 0x8E);
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base = (uint64_t)&idt;
    __asm__ volatile("lidt %0" :: "m"(idt_ptr));
}