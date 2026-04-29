section .text
bits 32

align 8
mb2_header_start:
    dd 0xE85250D6          ; magic
    dd 0                   ; arch (0 = i386)
    dd mb2_header_end - mb2_header_start
    dd -(0xE85250D6 + 0 + (mb2_header_end - mb2_header_start))

    ; FRAMEBUFFER TAG
    align 8
    dw 5                   ; type = framebuffer
    dw 0                   ; flags (0 = optional, don't crash if unsupported)
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

    ; Use explicit stack declared in BSS (safe, won't collide with GRUB/mem map)
    mov esp, stack_top

    ; Save multiboot2 info BEFORE we trash registers
    push ebx   ; multiboot2 info struct pointer
    push eax   ; multiboot2 magic

    ; -------------------------------------------------------
    ; Setup identity-map paging for long mode
    ; Tables at 0x1000–0x4FFF (well below kernel at 1MB)
    ; Maps first 4GB as 2MB pages (huge pages)
    ; -------------------------------------------------------

    ; Clear PML4 + PDPT + 4x PD = 6 tables * 4096 = 24576 bytes
    mov edi, 0x1000
    xor eax, eax
    mov ecx, 6 * 1024      ; 6 tables * 1024 dwords
    cld
    rep stosd

    ; PML4[0] -> PDPT at 0x2000
    mov dword [0x1000], 0x2003      ; present + write

    ; PDPT[0..3] -> PD[0..3] at 0x3000, 0x4000, 0x5000, 0x6000
    mov dword [0x2000], 0x3003
    mov dword [0x2008], 0x4003
    mov dword [0x2010], 0x5003
    mov dword [0x2018], 0x6003

    ; Fill each PD with 512 huge (2MB) page entries
    ; PD at 0x3000: maps 0x0000_0000 – 0x3FFF_FFFF
    mov edi, 0x3000
    xor ebx, ebx           ; physical base = 0
.fill_pd:
    mov eax, ebx
    or  eax, 0x83          ; present + write + huge
    mov [edi], eax
    ; high dword = 0 (already cleared)
    add edi, 8
    add ebx, 0x200000      ; next 2MB
    cmp edi, 0x7000        ; stop after 4 PDs (4*512*8 = 16384 bytes past 0x3000)
    jl  .fill_pd

    ; Load PML4
    mov eax, 0x1000
    mov cr3, eax

    ; Enable PAE (bit 5) and PGE (bit 7) in CR4
    mov eax, cr4
    or  eax, (1 << 5) | (1 << 7)
    mov cr4, eax

    ; Enable Long Mode in EFER MSR
    mov ecx, 0xC0000080
    rdmsr
    or  eax, (1 << 8)      ; LME
    wrmsr

    ; Enable paging (+ protected mode stays on) in CR0
    mov eax, cr0
    or  eax, (1 << 31)
    mov cr0, eax

    ; Far jump to flush pipeline and enter 64-bit code segment
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

    ; Args were pushed as 32-bit; zero-extend for 64-bit calling convention
    pop rdi    ; multiboot2 magic   -> arg1
    pop rsi    ; multiboot2 struct  -> arg2

    call kernel_main

    cli
.halt:
    hlt
    jmp .halt  ; in case NMI wakes us up

; -------------------------------------------------------
; GDT
; -------------------------------------------------------
align 16
gdt:
    dq 0                        ; null descriptor
    dq 0x0020980000000000       ; 64-bit code segment (ring 0)
    dq 0x0000920000000000       ; 64-bit data segment (ring 0)

gdtr:
    dw $ - gdt - 1
    dq gdt

; -------------------------------------------------------
; Stack (16KB, 16-byte aligned) in .bss
; -------------------------------------------------------
section .bss
align 16
    resb 16384
stack_top: