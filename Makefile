CC = gcc
AS = nasm

CFLAGS = -ffreestanding -O2 -Wall \
         -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
         -nostartfiles -nodefaultlibs \
         -mcmodel=kernel

LDFLAGS = -ffreestanding -nostdlib \
          -mno-red-zone -mno-mmx -mno-sse -mno-sse2 \
          -mcmodel=kernel

ASM_OBJS = kernel/boot.o kernel/isr.o
C_OBJS = kernel/kernel.o mm/kmalloc.o mm/paging.o sys/idt.o sys/exception.o \
         drivers/pic.o drivers/timer.o drivers/io.o \
         graphics/fb.o graphics/font.o fs/fs.o

ISO_NAME = FreeARS-0.03-UniversalFramebufferTest.iso

all: $(ISO_NAME)

kernel/boot.o: kernel/boot.asm
	$(AS) -f elf64 $< -o $@

kernel/isr.o: kernel/isr.asm
	$(AS) -f elf64 $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/kernel.bin: kernel/linker.ld $(ASM_OBJS) $(C_OBJS)
	$(CC) $(LDFLAGS) -T kernel/linker.ld -o $@ $(ASM_OBJS) $(C_OBJS) -lgcc

$(ISO_NAME): kernel/kernel.bin
	cp kernel/kernel.bin iso/boot/kernel.bin
	grub2-mkrescue -o $(ISO_NAME) iso

run: $(ISO_NAME)
	qemu-system-x86_64 -cdrom $(ISO_NAME) -m 256M -vga std -no-reboot -no-shutdown

clean:
	rm -f $(ASM_OBJS) $(C_OBJS) kernel/kernel.bin *.iso iso/boot/kernel.bin