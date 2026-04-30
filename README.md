Aqui está o README atualizado para a versão 0.04 com suporte UEFI e TSC:

```markdown
# FreeARS - Another Random System

> *"I'm doing a (free) operating system (just a hobby, won't be big and professional like linux)"*  
> — inspired by Linus Torvalds, 1991

FreeARS is a hobby x86_64 kernel written from scratch. Now with **UEFI boot**, **Limine protocol**, **TSC timing**, and tickless operation.

**Current version:** 0.04  
**Branch:** `64bit-uefi` (active development)

---

## Screenshots

*18:40 (6:40 PM) - 28/04/26 — IT BOOTED!!! WORKED ON 64 BIT MODE (QEMU) AFTER A COUPLE HOURS OF BUGS!*

*10:30 - 30/04/26 - IT BOOTED ON A BAREMETAL LIKE VM (VirtualBox)!!! Another win!!*

*20:00~ (08:00~ PM) - 31/04/26 - UEFI BOOT WITH LIMINE!!! TSC TIMING WORKING!!!*

*20:00~ (08:00~ PM) - 31/04/26 - BOOT ON VIRTUAL BOX!*

### UEFI Boot + fastfetch + TSC (QEMU)
![UEFI Boot](pictures/FreeARS_UEFI.png)

### UEFI Boot + Successfull ram detection! (Virtual Box!)
![UEFI Boot VB](pictures/FreeARS_UEFI_VB.png)

---

## What's new in 0.04

- **UEFI boot** via Limine bootloader (no more GRUB/BIOS)
- **TSC (Time Stamp Counter) timing** — microsecond precision
- **Tickless operation** — no more PIT IRQ jitter
- **TSC calibration** via PIT (automatic frequency detection)
- **Accurate sleep_ms()** — now sleeps exactly 1ms, not 10ms
- **Better uptime tracking** — nanosecond resolution
- **Limine protocol** — proper framebuffer detection on UEFI
- **Removed PIT IRQ dependency** — cleaner interrupt handling
- Works on **UEFI systems** (real hardware {MAYBE? Tests upcoming} + QEMU OVMF)
- Also, credits to the limine-c-template!! I'm using the same license. I couldn't do it without a template, sadly.

---

## Features

- **UEFI boot** (Limine bootloader)
- x86_64 long mode
- Universal framebuffer (via Limine protocol)
- Graphical shell with 8x16 bitmap font
- **TSC-based timing** (calibrated via PIT)
- Real CPU name detection via CPUID + RAM Detection
- Commands: `help`, `clear`, `uname`, `echo`, `sleep`, `crash`, `ticks`, `fastfetch`, `reboot`
- Dynamic memory allocator (`kmalloc`/`kfree`)
- 4-level paging (PML4)
- IDT with graphical exception handler
- PS/2 Keyboard polling with Shift/Caps Lock
- Custom ASCII art boot screen

---

## Performance: PIT vs TSC

| Aspect | PIT (0.03) | TSC (0.04) |
|--------|------------|------------|
| Resolution | 10ms | ~1ns |
| sleep_ms(1) | 10-20ms | 1ms exact |
| Read overhead | ~500 cycles | ~25 cycles |
| IRQ overhead | Yes (16 regs) | None |
| Jitter | ±5ms | ±0.001ms |

---

## What it lacks (still)

- User mode (ring 3)
- Multitasking / Scheduler (Hate this part bru)
- Mouse support
- Networking
- GPU drivers
- Real filesystem (FAT32/ext2)
- Any practical use (still just a hobby)

---

## Commands

| Command | Description |
|---------|-------------|
| `help` | Show all commands |
| `clear` | Clear screen |
| `uname` | System info |
| `echo <text>` | Print text |
| `sleep <ms>` | Sleep milliseconds (exact with TSC) |
| `crash` | Test exception handler |
| `ticks` | Show uptime ticks (100Hz equivalent) |
| `fastfetch` | System info with CPU name and TSC freq |
| `reboot` | Reboot system (works on QEMU/hardware) |

---

## Bootloaders

| Version | Bootloader | Mode | Status |
|---------|-----------|------|--------|
| 0.01 (32bit-unused) | GRUB | BIOS/Legacy | Deprecated |
| 0.02 | GRUB (64bit) (Multiboot2) | BIOS/Legacy | Deprecated |
| 0.03 | GRUB (64bit) (Multiboot2) | BIOS/Legacy | Old stable |
| **0.04** | **(64bit-uefi) Limine** | **UEFI** | **Current** |

---

## History

| Version | Description |
|---------|-------------|
| 0.01 | First release. 32-bit, VESA, Legacy BIOS |
| 0.02 | 64-bit, Multiboot2, framebuffer, shell |
| 0.03 | Universal FB detection, CPUID, RAM Disk FS, VirtualBox support |
| **0.04** | **UEFI (Limine), TSC timing, tickless operation** |

---

## Next steps (0.05)

- [ ] APIC timer for preemptive multitasking
- [ ] Basic scheduler (round-robin)
- [ ] User mode (ring 3)
- [ ] System calls
- [ ] FAT32 driver
- [ ] PCI enumeration

---

## License

Do whatever you want. It's a hobby.

