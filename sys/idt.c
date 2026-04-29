#include "idt.h"
static idt_entry_t idt[256]; static idt_ptr_t idt_ptr;

extern void isr0(),isr1(),isr2(),isr3(),isr4(),isr5(),isr6(),isr7(),isr8(),isr9();
extern void isr10(),isr11(),isr12(),isr13(),isr14(),isr15(),isr16(),isr17(),isr18(),isr19();
extern void isr20(),isr21(),isr22(),isr23(),isr24(),isr25(),isr26(),isr27(),isr28(),isr29(),isr30(),isr31();
extern void irq0();

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
    set(1, (uint64_t)isr1, 0x08, 0x8E);
    set(2, (uint64_t)isr2, 0x08, 0x8E);
    set(3, (uint64_t)isr3, 0x08, 0x8E);
    set(4, (uint64_t)isr4, 0x08, 0x8E);
    set(5, (uint64_t)isr5, 0x08, 0x8E);
    set(6, (uint64_t)isr6, 0x08, 0x8E);
    set(7, (uint64_t)isr7, 0x08, 0x8E);
    set(8, (uint64_t)isr8, 0x08, 0x8E);
    set(9, (uint64_t)isr9, 0x08, 0x8E);
    set(10, (uint64_t)isr10, 0x08, 0x8E);
    set(11, (uint64_t)isr11, 0x08, 0x8E);
    set(12, (uint64_t)isr12, 0x08, 0x8E);
    set(13, (uint64_t)isr13, 0x08, 0x8E);
    set(14, (uint64_t)isr14, 0x08, 0x8E);
    set(15, (uint64_t)isr15, 0x08, 0x8E);
    set(16, (uint64_t)isr16, 0x08, 0x8E);
    set(17, (uint64_t)isr17, 0x08, 0x8E);
    set(18, (uint64_t)isr18, 0x08, 0x8E);
    set(19, (uint64_t)isr19, 0x08, 0x8E);
    set(20, (uint64_t)isr20, 0x08, 0x8E);
    set(21, (uint64_t)isr21, 0x08, 0x8E);
    set(22, (uint64_t)isr22, 0x08, 0x8E);
    set(23, (uint64_t)isr23, 0x08, 0x8E);
    set(24, (uint64_t)isr24, 0x08, 0x8E);
    set(25, (uint64_t)isr25, 0x08, 0x8E);
    set(26, (uint64_t)isr26, 0x08, 0x8E);
    set(27, (uint64_t)isr27, 0x08, 0x8E);
    set(28, (uint64_t)isr28, 0x08, 0x8E);
    set(29, (uint64_t)isr29, 0x08, 0x8E);
    set(30, (uint64_t)isr30, 0x08, 0x8E);
    set(31, (uint64_t)isr31, 0x08, 0x8E);
    
    set(0x20, (uint64_t)irq0, 0x08, 0x8E);
    
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base = (uint64_t)&idt;
    __asm__ volatile("lidt %0" :: "m"(idt_ptr));
}