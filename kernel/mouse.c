#include "mouse.h"
#include "ports.h"
#include "idt.h"
#include "gfx.h"

static u8 cycle = 0;
static u8 packet[3];
static int mx = 400, my = 300;
static bool left = false, right = false;
static bool left_was = false;
static bool left_press = false, left_release = false;
static int screen_w = 1024, screen_h = 768;

static void mouse_wait(u8 type) {
    u32 timeout = 100000;
    if (type == 0) {
        while (timeout--) {
            if ((inb(0x64) & 1) == 1) return;
        }
    } else {
        while (timeout--) {
            if ((inb(0x64) & 2) == 0) return;
        }
    }
}

static void mouse_write(u8 val) {
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, val);
}

static u8 mouse_read(void) {
    mouse_wait(0);
    return inb(0x60);
}

void mouse_handler(void) {
    u8 data = inb(0x60);
    if (cycle == 0) {
        if (!(data & 0x08)) return; /* sync bit */
        packet[0] = data;
        cycle = 1;
    } else if (cycle == 1) {
        packet[1] = data;
        cycle = 2;
    } else {
        packet[2] = data;
        cycle = 0;

        left = (packet[0] & 1) != 0;
        right = (packet[0] & 2) != 0;

        i8 dx = (i8)packet[1];
        i8 dy = (i8)packet[2];
        if (packet[0] & 0x40) dx = 0; /* overflow */
        if (packet[0] & 0x80) dy = 0;

        mx += dx;
        my -= dy;
        if (mx < 0) mx = 0;
        if (my < 0) my = 0;
        if (mx >= screen_w) mx = screen_w - 1;
        if (my >= screen_h) my = screen_h - 1;
    }
}

void mouse_init(void) {
    screen_w = (int)gfx_width();
    screen_h = (int)gfx_height();
    mx = screen_w / 2;
    my = screen_h / 2;

    mouse_wait(1); outb(0x64, 0xA8);
    mouse_wait(1); outb(0x64, 0x20);
    mouse_wait(0);
    u8 status = inb(0x60) | 2;
    mouse_wait(1); outb(0x64, 0x60);
    mouse_wait(1); outb(0x60, status);

    mouse_write(0xF6); mouse_read();
    mouse_write(0xF4); mouse_read();

    irq_install_handler(12, mouse_handler);
}

int mouse_x(void) { return mx; }
int mouse_y(void) { return my; }
bool mouse_left(void) { return left; }
bool mouse_right(void) { return right; }

void mouse_poll_buttons(void) {
    left_press = left && !left_was;
    left_release = !left && left_was;
    left_was = left;
}

bool mouse_left_pressed(void) { return left_press; }
bool mouse_left_released(void) { return left_release; }
