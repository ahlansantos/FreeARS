bits 32
section .text

extern exception_handler
extern timer_handler

global isr0
global isr6
global isr8
global isr13
global isr14
global irq0

isr0:
    cli
    mov esp, 0x90000
    push dword 0
    push dword 0
    pushad
    push esp
    call exception_handler
    jmp $

isr6:
    cli
    mov esp, 0x90000
    push dword 0
    push dword 6
    pushad
    push esp
    call exception_handler
    jmp $

isr8:
    cli
    mov esp, 0x90000
    push dword 0
    push dword 8
    pushad
    push esp
    call exception_handler
    jmp $

isr13:
    cli
    mov esp, 0x90000
    push dword 13
    pushad
    push esp
    call exception_handler
    jmp $

isr14:
    cli
    mov esp, 0x90000
    push dword 14
    pushad
    push esp
    call exception_handler
    jmp $

irq0:
    cli
    pushad
    call timer_handler
    popad
    sti
    iret