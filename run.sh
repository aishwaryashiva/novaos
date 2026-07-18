#!/usr/bin/env bash
# Run NovaOS on your PC with QEMU
set -euo pipefail
cd "$(dirname "$0")"

IMG="novaos.img"
if [[ ! -f "$IMG" ]]; then
  echo "Building $IMG..."
  make
fi

if ! command -v qemu-system-i386 >/dev/null 2>&1; then
  echo "QEMU not found. Install it first:"
  echo "  Ubuntu/Debian: sudo apt-get install qemu-system-x86"
  echo "  Fedora:        sudo dnf install qemu-system-x86"
  echo "  macOS:         brew install qemu"
  exit 1
fi

echo "Starting NovaOS (close the QEMU window to quit)..."
exec qemu-system-i386 \
  -drive "format=raw,file=$IMG" \
  -m 64 \
  -vga std \
  -name NovaOS
