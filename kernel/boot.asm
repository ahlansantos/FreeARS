section .text.boot
bits 32

align 8
mb2_header_start:
    dd 0xE85250D6
    dd 0
    dd mb2_header_end - mb2_header_start
    dd -(0xE85250D6 + 0 + (mb2_header_end - mb2_header_start))
    align 8
    dw 5
    dw 0
    dd 20
    dd 800
    dd 600
    dd 32
    align 8
    dw 0
    dw 0
    dd 8
mb2_header_end:

extern kernel_main
global _start

_start:
    cli
    mov esp, stack_top
    push ebx
    push eax
    mov edi, 0x1000
    xor eax, eax
    mov ecx, 6 * 1024
    cld
    rep stosd
    mov dword [0x1000], 0x2003
    mov dword [0x2000], 0x3003
    mov dword [0x2008], 0x4003
    mov dword [0x2010], 0x5003
    mov dword [0x2018], 0x6003
    mov edi, 0x3000
    xor ebx, ebx
.fill_pd:
    mov eax, ebx
    or  eax, 0x83
    mov [edi], eax
    add edi, 8
    add ebx, 0x200000
    cmp edi, 0x7000
    jl  .fill_pd
    mov eax, 0x1000
    mov cr3, eax
    mov eax, cr4
    or  eax, (1 << 5) | (1 << 7)
    mov cr4, eax
    mov ecx, 0xC0000080
    rdmsr
    or  eax, (1 << 8)
    wrmsr
    mov eax, cr0
    or  eax, (1 << 31)
    mov cr0, eax
    lgdt [gdtr]
    jmp 0x08:long_mode

bits 64
long_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    pop rdi
    pop rsi
    call kernel_main
    cli
.halt:
    hlt
    jmp .halt

align 16
gdt:
    dq 0
    dq 0x0020980000000000
    dq 0x0000920000000000

gdtr:
    dw $ - gdt - 1
    dq gdt

section .bss
align 16
    resb 16384
stack_top: