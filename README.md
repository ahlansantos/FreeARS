# FreeARS - Another Random System

> *"I'm doing a (free) operating system (just a hobby, won't be big and professional like linux)"*  
> — inspired by Linus Torvalds, 1991

FreeARS is a hobby x86 kernel written from scratch. It boots, has a shell, handles interrupts, and doesn't catch fire (most of the time).

**Current version:** 0.01

---

## Some screenshots

![Help cmd](https://imgur.com/E3w06NN.png)
---

## Features (as of 0.01)

- Booting via GRUB (Multiboot)
- 32-bit protected mode
- VGA text console with scrolling
- Interactive shell
- Dynamic memory allocator (`kmalloc`/`kfree` with splitting & coalescing)
- Paging (identity-mapped first 16 MB)
- IDT with exception handlers
- PIC 8259 remapped
- PIT timer at 100 Hz
- PS/2 keyboard via IRQ1 with circular buffer
- Fake package manager (`arpm`)

---

## What it lacks

- A filesystem
- User mode
- Multitasking
- Networking (lol)
- Any practical use whatsoever

---

## Building

You'll need an i686-elf cross-compiler, nasm, and grub-mkrescue.

```bash
$ make          # builds freeARS.iso
$ make run      # runs in QEMU
$ make clean    # cleans up
```

---

## Running on real hardware

```bash
$ dd if=freeARS.iso of=/dev/sdX bs=1M status=progress
```

Or use [Ventoy](https://ventoy.net).

**Note:** Requires Legacy BIOS boot. UEFI is not supported yet.

---

## Commands

| Command | What it does |
|---------|-------------|
| `help` | Shows available commands |
| `clear` | Clears the screen |
| `uname` | Prints system info |
| `memtest` | Tests memory allocator |
| `pagetest` | Tests paging |
| `crash` | Forces a division by zero (tests exception handling) |
| `ticks` | Shows timer tick count |
| `arpm list` | Lists fake packages |
| `arpm -ci <pkg>` | "Installs" a fake package |

---

## Why?

Because every programmer should write an OS at least once.  
Also, because I wanted to see if I could make `crash` print a pretty red screen.

---

## License

MIT
Do whatever you want. If you make something cool, let me know.
