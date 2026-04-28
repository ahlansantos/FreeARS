section .text
bits 32

align 8
mb2_header_start:
    dd 0xE85250D6          ; magic
    dd 0                   ; arch (0 = i386)
    dd mb2_header_end - mb2_header_start
    dd -(0xE85250D6 + 0 + (mb2_header_end - mb2_header_start))

    ; FRAMEBUFFER TAG PLS WORKK
    align 8
    dw 5                   ; type = framebuffer
    dw 0                   ; flags
    dd 20                  ; size
    dd 800                 ; width
    dd 600                 ; height
    dd 32                  ; depth

    ; END TAG
    align 8
    dw 0
    dw 0
    dd 8
mb2_header_end:

extern kernel_main
global _start

_start:
    cli
    mov esp, 0x90000
    push ebx
    push eax
    
    mov edi, 0x1000
    mov cr3, edi
    xor eax, eax
    mov ecx, 4096
    cld
    rep stosd
    
    mov dword [0x1000], 0x2003
    mov dword [0x2000], 0x3003
    mov dword [0x3000], 0x0083
    mov dword [0x3008], 0x200083
    
    mov eax, cr4
    or eax, 0xA0
    mov cr4, eax
    
    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x100
    wrmsr
    
    mov eax, cr0
    or eax, 0x80000000
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
    hlt

align 16
gdt:
    dq 0
    dq 0x0020980000000000
    dq 0x0000920000000000

gdtr:
    dw $ - gdt - 1
    dq gdt