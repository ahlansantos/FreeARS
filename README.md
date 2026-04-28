# FreeARS - Another Random System

> *"I'm doing a (free) operating system (just a hobby, won't be big and professional like linux)"*  
> — inspired by Linus Torvalds, 1991

FreeARS is a hobby x86_64 kernel written from scratch. Now with UEFI support and 64-bit mode, aiming to run on modern hardware.

**Current version:** 0.02  
**Branch:** `64bit` (active development)

---

## Screenshots

*Coming soon, once the framebuffer works on real hardware!*

---

## What's new in 0.02

- **x86_64 (64-bit)** protected mode
- **UEFI boot** via Multiboot2 (GOP framebuffer)
- 4-level paging (PML4)
- Rewritten from 32-bit codebase
- Targeting modern GPUs (GOP instead of VESA VBE)
- Cross-compiler: `x86_64-elf-gcc`

---

## Features

- Multiboot2 (GRUB) with UEFI support
- 64-bit long mode
- GOP framebuffer with bitmap font
- Graphical shell with scroll
- Commands: help, clear, uname, echo, sleep, memtest, pagetest, crash, ticks, fastfetch, arpm
- Dynamic memory allocator (kmalloc/kfree)
- 4-level paging
- IDT with graphical exception handler
- APIC/8259 + Timer (100 Hz)
- PS/2 Keyboard polling with Shift/Caps Lock
- Custom ASCII art boot screen

---

## What it lacks

- Filesystem
- User mode (ring 3)
- Multitasking
- Mouse
- Networking
- GPU drivers
- Any practical use

---

## Hardware

Tested on QEMU. Should work on UEFI hardware with GOP.

```bash
dd if=freeARS.iso of=/dev/sdX bs=1M status=progress
```

Or use [Ventoy](https://ventoy.net). Requires **UEFI boot** (no Legacy BIOS).

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

## History

| Version | Branch | Description |
|---------|--------|-------------|
| 0.01 | `32bit` | First release. 32-bit, VESA, Legacy BIOS |
| 0.02 | `64bit` | Current. 64-bit, UEFI, GOP |

---

## License

MIT
```
