# FreeARS - Another Random System
> *"I'm doing a (free) operating system (just a hobby, won't be big and professional like linux)"*  
> — inspired by Linus Torvalds, 1991

FreeARS is a hobby x86_64 kernel written from scratch. UEFI boot via Limine, TSC-based timing, and now a working physical memory manager.

**Current version:** 0.05  
**Branch:** `x86_64-uefi` (active development)

---

## Screenshots

*28/04/26 — IT BOOTED!!! 64-bit mode (QEMU) after hours of bugs!*  
*30/04/26 — Booted on a baremetal-like VM (VirtualBox)!*  
*31/04/26 — UEFI + Limine + TSC working!!!*  
*31/04/26 — Bare metal on real hardware working! Posted on my tiktok. @theloneahlan*  
*01/05/26 — PMM + heap working! Tested up to 32GB RAM.*

### UEFI Boot + fastfetch + PMM + TSC Ticks!! (QEMU)
![UEFI Boot PMM](__pictures/FreeARS-0.05-PMM.png__)

### UEFI Boot + fastfetch + TSC (QEMU)
![UEFI Boot](__pictures/FreeARS_UEFI.png__)

### UEFI Boot + RAM detection (VirtualBox)
![UEFI Boot VB](__pictures/FreeARS_UEFI_VB.png__)

---

## What's new in 0.05

- **Physical Memory Manager (PMM)** -> bitmap allocator, works with any RAM size
- **HHDM-aware allocation** -> uses Limine's Higher Half Direct Map offset correctly
- **Heap allocator** (`kmalloc` / `kfree`) with block splitting and coalescing
- **Robust memmap parsing** -> ignores PCI MMIO holes, only tracks usable RAM
- Tested with **2GB and 32GB** in QEMU, boots correctly on both
- Serial debug output during PMM init for easier crash diagnosis
- Better shell command: `memtest` -> tests PMM and heap allocation live

---

## Features

- **UEFI boot** (Limine)
- x86_64 long mode
- Framebuffer rendering
- Basic graphical shell (8x16 bitmap font)
- TSC-based timing (calibrated via PIT)
- CPUID CPU name detection
- RAM detection and usable memory tracking
- **Physical memory manager** (bitmap)
- **Heap allocator** (`kmalloc` / `kfree`)
- IDT + basic exception handling
- PS/2 keyboard input (polling, Shift/Caps support)
- Custom ASCII boot screen
- Serial debug output

### Commands

| Command | Description |
|---------|-------------|
| `help` | Show commands |
| `clear` | Clear screen |
| `uname` | System info |
| `echo <text>` | Print text |
| `sleep <ms>` | Sleep using TSC |
| `ticks` | Show uptime |
| `fastfetch` | System overview |
| `memtest` | Test PMM and heap |
| `crash` | Trigger exception | -> May not work on 0.04+? TODO: FIX LATER
| `reboot` | Reboot system |

---

## Memory management (0.05)

The PMM uses a bitmap where each bit represents one 4KB page. Only pages marked `USABLE` by Limine are freed; everything else stays reserved. The HHDM offset from Limine is used to access physical pages through virtual addresses, no direct physical access.

The heap sits on top of the PMM, allocating pages on demand. `kfree` coalesces adjacent free blocks to prevent fragmentation.

Supports any amount of RAM (0.04 supported 94mb~ in PMM) tested up to 32GB. PCI MMIO holes (which can appear above 256GB on some boards) are correctly ignored when sizing the bitmap.

---

## Performance: PIT vs TSC

| Aspect | PIT (0.03) | TSC (0.04+) |
|--------|------------|-------------|
| Resolution | ~10ms | ~1ns |
| sleep_ms(1) | ~10–20ms | ~1ms |
| Read cost | High | Very low |
| IRQ usage | Yes | No |
| Jitter | High | Minimal |

---

## Bootloader history

| Version | Bootloader | Mode | Status |
|---------|-----------|------|--------|
| 0.01 | GRUB | BIOS/Legacy | Deprecated |
| 0.02 | GRUB (Multiboot2) | BIOS/Legacy | Deprecated |
| 0.03 | GRUB (Multiboot2) | BIOS/Legacy | Old stable |
| 0.04–0.05 | **Limine** | **UEFI** | **Current** |

---

## Version history

| Version | Description |
|---------|-------------|
| 0.01 | 32-bit, VESA, BIOS |
| 0.02 | 64-bit, Multiboot2, shell |
| 0.03 | Framebuffer, CPUID, RAM detection |
| 0.04 | UEFI + Limine, TSC timing, tickless kernel |
| **0.05** | **PMM bitmap, heap allocator, HHDM-aware memory** |

---

## Next steps (0.06)

- [ ] Virtual memory manager (VMM) — map arbitrary virtual pages
- [ ] APIC timer (replace TSC busy-wait for sleep)
- [ ] Basic round-robin scheduler
- [ ] User mode (ring 3)
- [ ] Syscalls
- [ ] PCI enumeration
- [ ] FAT32 driver

---

## Notes

- TSC is calibrated using PIT for accuracy; busy-wait is used for sleep (no timer IRQ yet)
- PMM serial debug logs are printed during boot --- use `-serial stdio` in QEMU to see them
- Designed purely for learning low-level systems programming!!

---

## License

Do whatever you want. It's a hobby.
