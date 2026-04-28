# FreeARS - Another Random System

> *"I'm doing a (free) operating system (just a hobby, won't be big and professional like linux)"*  
> — inspired by Linus Torvalds, 1991

FreeARS is a hobby x86 kernel written from scratch. It boots, has a graphical shell, handles interrupts, and doesn't catch fire (most of the time).

**Current version:** 0.01

---

## Screenshots

**Shell with VESA framebuffer (1280x720)**
![Shell](https://imgur.com/BA2rVzQ.png)

---

## Features (as of 0.01)

- Booting via GRUB (Multiboot)
- 32-bit protected mode
- VESA framebuffer (1280x720x32) with bitmap font
- Graphical shell with scroll support
- Interactive commands (help, echo, sleep, clear, fastfetch)
- VGA text mode fallback (if VESA unavailable)
- Dynamic memory allocator (`kmalloc`/`kfree` with splitting & coalescing)
- Paging (4MB pages, PSE enabled)
- IDT with graphical exception handler (shows registers on crash)
- PIC 8259 remapped
- PIT timer at 100 Hz
- PS/2 keyboard via IRQ1 with Shift/Caps Lock support
- Fake package manager (`arpm`)
- Custom ASCII art boot screen

---

## What it lacks

- A filesystem
- User mode
- Multitasking
- Mouse support (coming soon)
- Networking (lol)
- Any practical use whatsoever
- GPU Drivers? I'll try to add it.

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
| `echo <text>` | Prints text |
| `sleep <ms>` | Sleeps for milliseconds |
| `memtest` | Tests memory allocator |
| `pagetest` | Tests paging |
| `crash` | Forces exception (tests graphical exception handler) |
| `ticks` | Shows timer tick count |
| `fastfetch` | System info display |
| `arpm list` | Lists fake packages |
| `arpm -ci <pkg>` | "Installs" a fake package |

---

## Why?

Because every programmer should write an OS at least once.  
Also, because I wanted to see if I could make `crash` print a pretty red screen.

---

## License

MIT — do whatever you want. If you make something cool, let me know.
