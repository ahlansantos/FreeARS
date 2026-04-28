CC = gcc
AS = nasm
CFLAGS = -ffreestanding -O2 -Wall -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -nostartfiles -nodefaultlibs

ASM_OBJS = kernel/boot.o kernel/isr.o
C_OBJS = kernel/kernel.o mm/kmalloc.o mm/paging.o sys/idt.o sys/exception.o drivers/pic.o drivers/timer.o drivers/io.o graphics/fb.o graphics/font.o

all: freeARS.iso

kernel/boot.o: kernel/boot.asm
	$(AS) -f elf64 $< -o $@

kernel/isr.o: kernel/isr.asm
	$(AS) -f elf64 $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/kernel.bin: kernel/linker.ld $(ASM_OBJS) $(C_OBJS)
	$(CC) $(CFLAGS) -T kernel/linker.ld -o $@ -nostdlib $(ASM_OBJS) $(C_OBJS) -lgcc

freeARS.iso: kernel/kernel.bin
	cp kernel/kernel.bin iso/boot/kernel.bin
	grub2-mkrescue -o freeARS.iso iso

run: freeARS.iso
	qemu-system-x86_64 -cdrom freeARS.iso -m 256M -vga std -no-reboot -no-shutdown
	
clean:
	rm -f $(ASM_OBJS) $(C_OBJS) kernel/kernel.bin freeARS.iso iso/boot/kernel.bin













	