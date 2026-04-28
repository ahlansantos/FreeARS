bits 64
section .text

extern exception_handler
extern timer_handler

global isr0, isr6, isr8, isr13, isr14, irq0

%macro pushaq 0
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15
%endmacro

%macro popaq 0
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
%endmacro

isr0:
    cli
    push 0
    push 0
    pushaq
    mov rdi, rsp
    call exception_handler
    jmp $

isr6:
    cli
    push 0
    push 6
    pushaq
    mov rdi, rsp
    call exception_handler
    jmp $

isr8:
    cli
    push 0
    push 8
    pushaq
    mov rdi, rsp
    call exception_handler
    jmp $

isr13:
    cli
    push 13
    pushaq
    mov rdi, rsp
    call exception_handler
    jmp $

isr14:
    cli
    push 14
    pushaq
    mov rdi, rsp
    call exception_handler
    jmp $

irq0:
    cli
    pushaq
    call timer_handler
    popaq
    sti
    iretq