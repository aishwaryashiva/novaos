# NovaOS — bare-metal GUI operating system
# Build tools only (nasm/gcc/ld). No OS SDK, no existing distribution.

AS      = nasm
CC      = gcc
LD      = ld

CFLAGS  = -m32 -ffreestanding -fno-builtin -fno-stack-protector \
          -fno-pic -nostdlib -nostdinc -Wall -Wextra -O2 \
          -Iinclude -std=c11
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

KERNEL_OBJS = \
	kernel/entry.o \
	kernel/kernel.o \
	kernel/string.o \
	kernel/idt.o \
	kernel/font.o \
	kernel/gfx.o \
	kernel/keyboard.o \
	kernel/mouse.o \
	kernel/gui.o

.PHONY: all clean run run-sdl disk

all: novaos.img

boot/boot.bin: boot/boot.asm
	$(AS) -f bin $< -o $@

boot/stage2.bin: boot/stage2.asm
	$(AS) -f bin $< -o $@

kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel/entry.o: kernel/entry.asm
	$(AS) $(ASFLAGS) $< -o $@

kernel/kernel.bin: $(KERNEL_OBJS) linker.ld
	$(LD) $(LDFLAGS) -o kernel/kernel.elf $(KERNEL_OBJS)
	objcopy -O binary kernel/kernel.elf $@

novaos.img: boot/boot.bin boot/stage2.bin kernel/kernel.bin
	dd if=/dev/zero of=$@ bs=512 count=2880 status=none
	dd if=boot/boot.bin of=$@ conv=notrunc status=none
	dd if=boot/stage2.bin of=$@ bs=512 seek=1 conv=notrunc status=none
	dd if=kernel/kernel.bin of=$@ bs=512 seek=9 conv=notrunc status=none
	@echo "Built $@"

run: novaos.img
	qemu-system-i386 -drive format=raw,file=novaos.img -m 64 -vga std -display none -serial stdio

# Headless smoke test: boot for a few seconds and capture serial if any
test: novaos.img
	timeout 5 qemu-system-i386 -drive format=raw,file=novaos.img -m 64 -vga std \
		-display none -serial mon:stdio -no-reboot -no-shutdown || true
	@echo "QEMU smoke test finished"

# Local graphical run (when a display is available)
run-sdl: novaos.img
	qemu-system-i386 -drive format=raw,file=novaos.img -m 64 -vga std -display sdl

clean:
	rm -f boot/*.bin kernel/*.o kernel/*.elf kernel/*.bin novaos.img
