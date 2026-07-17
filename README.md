# NovaOS

A real bare-metal GUI operating system for x86, written from scratch.

No Linux. No existing OS distribution. No GUI toolkit or OS SDK.  
Only a custom bootloader, kernel, drivers, and window manager — plus ordinary
build tools (`nasm`, `gcc`, `ld`) and QEMU for running the disk image.

## What it is

NovaOS boots on BIOS/QEMU, switches into protected mode, sets a VESA linear
framebuffer, and runs a software-composited desktop with mouse and keyboard.

| Layer | Implementation |
|--------|----------------|
| Boot | Custom MBR + stage-2 (A20, VESA, GDT, protected mode) |
| Kernel | 32-bit freestanding C + assembly at `0x100000` |
| Interrupts | IDT + PIC remapping, PIT timer |
| Graphics | VESA LFB double-buffered software renderer |
| Input | PS/2 keyboard & mouse |
| GUI | Desktop, taskbar, start menu, draggable windows |
| Apps | About, Terminal, Paint, Calculator |

## Build

```bash
sudo apt-get install nasm gcc-multilib qemu-system-x86
make
```

Produces `novaos.img` (1.44M raw disk image).

## Run

```bash
# Graphical (local display)
make run-sdl

# Or:
qemu-system-i386 -drive format=raw,file=novaos.img -m 64 -vga std
```

Serial console (debug lines from the kernel):

```bash
qemu-system-i386 -drive format=raw,file=novaos.img -m 64 -vga std -serial stdio
```

## Use

- Click desktop icons or **NovaOS** (taskbar) to open apps
- Drag title bars to move windows; click **x** to close
- **Terminal**: `help`, `clear`, `about`, `uname`, `echo …`
- **Paint**: pick a color, drag to draw
- **Calculator**: integer keypad

## Layout

```
boot/boot.asm      Stage-1 MBR
boot/stage2.asm    VESA + protected mode + kernel load
kernel/entry.asm   Kernel entry + ISR/IRQ stubs
kernel/kernel.c    Bring-up
kernel/idt.c       Interrupt descriptor table
kernel/gfx.c       Framebuffer / font / cursor
kernel/keyboard.c  PS/2 keyboard
kernel/mouse.c     PS/2 mouse
kernel/gui.c       Desktop, WM, apps
include/           Public headers
linker.ld          Kernel link script
Makefile
```

## Notes

This is a teaching / hobby OS: single address space, cooperative main loop,
no userspace processes or filesystem yet. It is a real machine-mode OS image,
not a web or desktop simulation.
