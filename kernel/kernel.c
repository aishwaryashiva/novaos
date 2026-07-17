#include "types.h"
#include "gfx.h"
#include "idt.h"
#include "keyboard.h"
#include "mouse.h"
#include "gui.h"
#include "ports.h"

static void serial_init(void) {
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x80);
    outb(0x3F8 + 0, 0x03);
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03);
    outb(0x3F8 + 2, 0xC7);
    outb(0x3F8 + 4, 0x0B);
}

static void serial_put(char c) {
    while ((inb(0x3F8 + 5) & 0x20) == 0);
    outb(0x3F8, (u8)c);
}

static void serial_print(const char* s) {
    while (*s) serial_put(*s++);
}

void kmain(boot_info_t* info) {
    serial_init();
    serial_print("NovaOS kernel enter\r\n");

    if (!info || !info->framebuffer || info->width == 0) {
        serial_print("FATAL: no framebuffer\r\n");
        for (;;) __asm__ volatile ("hlt");
    }

    serial_print("VESA OK — starting GUI\r\n");
    gfx_init(info);
    idt_init();
    keyboard_init();
    mouse_init();

    __asm__ volatile ("sti");

    gui_init();
    serial_print("NovaOS GUI running\r\n");
    gui_run();
}
