@echo off
REM Run NovaOS on Windows with QEMU
cd /d "%~dp0"

if not exist novaos.img (
  echo novaos.img not found. Build on Linux/WSL with "make", or download it from the repo.
  exit /b 1
)

where qemu-system-i386 >nul 2>&1
if errorlevel 1 (
  echo QEMU not found. Install from https://www.qemu.org/download/#windows
  echo Then add the QEMU folder to your PATH.
  exit /b 1
)

echo Starting NovaOS (close the QEMU window to quit)...
qemu-system-i386 -drive format=raw,file=novaos.img -m 64 -vga std -name NovaOS
