# FreeARS - Another Random System

> *"I'm doing a (free) operating system (just a hobby, won't be big and professional like linux)"*  
> — inspired by Linus Torvalds, 1991

FreeARS is a hobby x86 kernel written from scratch. It boots, has a graphical shell, handles interrupts, and doesn't catch fire (most of the time).

**Current version:** 0.01  
**Branch:** `32bit` (unused - unstable, final)

---

## Screenshots

![FreeARS Shell](https://imgur.com/DmO9xxf.png)

---

## Features

- Multiboot (GRUB)
- 32-bit protected mode
- VESA framebuffer (800x600x32 by default) with bitmap font
- Graphical shell with scroll
- Commands: help, clear, uname, echo, sleep, memtest, pagetest, crash, ticks, fastfetch, arpm
- Dynamic memory allocator (kmalloc/kfree)
- Paging with 4MB pages (PSE)
- IDT with graphical exception handler
- PIC 8259 + PIT timer (100 Hz)
- PS/2 Keyboard polling with Shift/Caps Lock
- Custom ASCII art boot screen

---

## What it lacks

- Filesystem
- User mode
- Multitasking
- Mouse
- Networking
- GPU drivers
- Any practical use

---

## Hardware

```bash
dd if=freeARS.iso of=/dev/sdX bs=1M status=progress
```

Or use [Ventoy](https://ventoy.net). Requires Legacy BIOS. UEFI not supported yet.

> **Note:** VESA may not work on modern GPUs (RTX 3050 etc). 

---

## Commands

| Command | Description |
|---------|-------------|
| `help` | Show commands |
| `clear` | Clear screen |
| `uname` | System info |
| `echo <text>` | Print text |
| `sleep <ms>` | Sleep |
| `memtest` | Test heap |
| `pagetest` | Test paging |
| `crash` | Test exception handler |
| `ticks` | Timer ticks |
| `fastfetch` | System info |
| `arpm list` | List packages |
| `arpm -ci <pkg>` | Install package |

---

## Future

This MAY be the **final 32-bit branch**. Work continues on the `64bit` branch with UEFI support to run on modern hardware.

---

## License

MIT
```
