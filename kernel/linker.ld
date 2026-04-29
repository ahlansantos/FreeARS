ENTRY(_start)

SECTIONS {
    . = 1M;

    .text : {
        KEEP(*(.text.boot))
        *(.text)
        *(.rodata)
    }

    . = ALIGN(4096);
    .data : {
        *(.data)
    }

    . = ALIGN(4096);
    .bss : {
        *(COMMON)
        *(.bss)
    }
}