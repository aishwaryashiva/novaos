# Run NovaOS on Windows (PowerShell) with QEMU
Set-Location $PSScriptRoot

if (-not (Test-Path "novaos.img")) {
    Write-Error "novaos.img not found. Build with make (WSL/Linux) or get it from the repo."
    exit 1
}

$qemu = Get-Command qemu-system-i386 -ErrorAction SilentlyContinue
if (-not $qemu) {
    Write-Host "QEMU not found. Install from https://www.qemu.org/download/#windows"
    Write-Host "Then add the QEMU install folder to PATH."
    exit 1
}

Write-Host "Starting NovaOS (close the QEMU window to quit)..."
& qemu-system-i386 -drive format=raw,file=novaos.img -m 64 -vga std -name NovaOS
