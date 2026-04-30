#include <stdint.h>
#include "keyboard.h"

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    asm volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

void keyboard_readline(char *buf, int max) {
    static const char lo[] = {0,0,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',0,' '};
    static const char up[] = {0,0,'!','@','#','$','%','^','&','*','(',')','_','+','\b','\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,'A','S','D','F','G','H','J','K','L',':','"','~',0,'|','Z','X','C','V','B','N','M','<','>','?',0,'*',0,' '};
    
    int i = 0, shift = 0, caps = 0;
    while (i < max - 1) {
        while (!(inb(0x64) & 1)) asm volatile("pause");
        uint8_t sc = inb(0x60);
        
        if (sc == 0x2A || sc == 0x36) { shift = 1; continue; }
        if (sc == 0xAA || sc == 0xB6) { shift = 0; continue; }
        if (sc == 0x3A) { caps = !caps; continue; }
        if (sc & 0x80) continue;
        
        char c = (shift ^ caps) ? up[sc] : lo[sc];
        if (!c) continue;
        if (c == '\n') break;
        if (c == '\b') {
            if (i > 0) {
                i--;
                extern void terminal_putchar(char c);
                terminal_putchar('\b');
            }
            continue;
        }
        buf[i++] = c;
        extern void terminal_putchar(char c);
        terminal_putchar(c);
    }
    buf[i] = '\0';
    extern void terminal_putchar(char c);
    terminal_putchar('\n');
}