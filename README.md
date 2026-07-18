# NovaOS

A real bare-metal GUI operating system for x86, written from scratch.

No Linux. No existing OS distribution. No GUI toolkit or OS SDK.  
Only a custom bootloader, kernel, drivers, and window manager — plus ordinary
build tools (`nasm`, `gcc`, `ld`) and **QEMU on your PC** for running it.

## Run on your PC (recommended)

### 1. Get the repo

```bash
git clone https://github.com/aishwaryashiva/novaos.git
cd novaos
git checkout cursor/novaos-gui-e71a
```

Or download the ZIP from GitHub and extract it.

### 2. Install QEMU

| OS | Install |
|----|---------|
| **Ubuntu/Debian** | `sudo apt-get install qemu-system-x86` |
| **Fedora** | `sudo dnf install qemu-system-x86` |
| **macOS** | `brew install qemu` |
| **Windows** | Install from [qemu.org/download](https://www.qemu.org/download/#windows) and add it to PATH |

### 3. Start NovaOS

The bootable image `novaos.img` is in the repo.

**Linux / macOS:**
```bash
chmod +x run.sh
./run.sh
```

**Windows (Command Prompt):**
```bat
run.bat
```

**Windows (PowerShell):**
```powershell
.\run.ps1
```

**Or manually:**
```bash
qemu-system-i386 -drive format=raw,file=novaos.img -m 64 -vga std
```

A QEMU window opens with the NovaOS desktop. Close that window to quit.

### 4. Use the desktop

- Click **About / Terminal / Paint / Calc** icons
- Drag windows by the title bar; click **x** to close
- Taskbar **NovaOS** opens the start menu
- **Terminal**: `help`, `clear`, `about`, `uname`, `echo …`

## Rebuild from source (optional)

Needed only if you change the code:

```bash
# Ubuntu/Debian
sudo apt-get install nasm gcc-multilib qemu-system-x86
make
./run.sh
```

## What it is

| Layer | Implementation |
|--------|----------------|
| Boot | Custom MBR + stage-2 (A20, VESA, GDT, protected mode) |
| Kernel | 32-bit freestanding C + assembly at `0x100000` |
| Interrupts | IDT + PIC remapping, PIT timer |
| Graphics | VESA LFB double-buffered software renderer |
| Input | PS/2 keyboard & mouse |
| GUI | Desktop, taskbar, start menu, draggable windows |
| Apps | About, Terminal, Paint, Calculator |

## Layout

```
boot/boot.asm      Stage-1 MBR
boot/stage2.asm    VESA + protected mode + kernel load
kernel/            Kernel, drivers, GUI
include/           Public headers
novaos.img         Bootable disk image (run this in QEMU)
run.sh / run.bat   One-click runners for your PC
Makefile
```

## Notes

This is a teaching / hobby OS: single address space, cooperative main loop,
no userspace processes or filesystem yet. It is a real machine-mode OS image,
not a web or desktop simulation. Always run it locally in QEMU on your PC.
