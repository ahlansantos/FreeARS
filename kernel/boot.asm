section .multiboot
align 4
    dd 0x1BADB002
    dd 0x07
    dd -(0x1BADB002+0x07)
    dd 0,0,0,0,0
    dd 1
    dd 800
    dd 600
    dd 32
section .text
extern kernel_main
global _start
_start:
    push ebx
    push eax
    call kernel_main
    hlt