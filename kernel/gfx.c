#include "gfx.h"
#include "string.h"

extern const u8* font_glyph(char c);

static u32* fb = 0;
static u32* back = 0;
static u32 width = 0;
static u32 height = 0;
static u32 pitch_px = 0;

/* Backbuffer in high memory after kernel */
#define BACKBUFFER_ADDR 0x200000

void gfx_init(boot_info_t* info) {
    width = info->width;
    height = info->height;
    pitch_px = info->pitch / 4;
    fb = (u32*)info->framebuffer;
    back = (u32*)BACKBUFFER_ADDR;
    gfx_clear(0x00101820);
}

u32 gfx_width(void)  { return width; }
u32 gfx_height(void) { return height; }
u32* gfx_backbuffer(void) { return back; }

void gfx_clear(u32 color) {
    u32 n = pitch_px * height;
    for (u32 i = 0; i < n; i++) back[i] = color;
}

void gfx_putpixel(int x, int y, u32 color) {
    if ((u32)x >= width || (u32)y >= height) return;
    back[y * pitch_px + x] = color;
}

u32 gfx_getpixel(int x, int y) {
    if ((u32)x >= width || (u32)y >= height) return 0;
    return back[y * pitch_px + x];
}

void gfx_fillrect(int x, int y, int w, int h, u32 color) {
    if (w <= 0 || h <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if ((u32)x >= width || (u32)y >= height) return;
    if ((u32)(x + w) > width) w = (int)width - x;
    if ((u32)(y + h) > height) h = (int)height - y;
    for (int row = 0; row < h; row++) {
        u32* line = &back[(y + row) * pitch_px + x];
        for (int col = 0; col < w; col++) line[col] = color;
    }
}

void gfx_hline(int x, int y, int w, u32 color) {
    gfx_fillrect(x, y, w, 1, color);
}

void gfx_vline(int x, int y, int h, u32 color) {
    gfx_fillrect(x, y, 1, h, color);
}

void gfx_drawrect(int x, int y, int w, int h, u32 color) {
    gfx_hline(x, y, w, color);
    gfx_hline(x, y + h - 1, w, color);
    gfx_vline(x, y, h, color);
    gfx_vline(x + w - 1, y, h, color);
}

void gfx_fillround(int x, int y, int w, int h, int r, u32 color) {
    if (r <= 0) { gfx_fillrect(x, y, w, h, color); return; }
    gfx_fillrect(x + r, y, w - 2 * r, h, color);
    gfx_fillrect(x, y + r, r, h - 2 * r, color);
    gfx_fillrect(x + w - r, y + r, r, h - 2 * r, color);
    for (int dy = 0; dy < r; dy++) {
        for (int dx = 0; dx < r; dx++) {
            int d2 = (r - 1 - dx) * (r - 1 - dx) + (r - 1 - dy) * (r - 1 - dy);
            if (d2 <= r * r) {
                gfx_putpixel(x + dx, y + dy, color);
                gfx_putpixel(x + w - 1 - dx, y + dy, color);
                gfx_putpixel(x + dx, y + h - 1 - dy, color);
                gfx_putpixel(x + w - 1 - dx, y + h - 1 - dy, color);
            }
        }
    }
}

void gfx_draw_char(int x, int y, char c, u32 fg, u32 bg) {
    const u8* g = font_glyph(c);
    for (int row = 0; row < 16; row++) {
        u8 bits = g[row];
        for (int col = 0; col < 8; col++) {
            u32 color = (bits & (0x80 >> col)) ? fg : bg;
            if (bg == 0xFFFFFFFF && !(bits & (0x80 >> col))) continue;
            gfx_putpixel(x + col, y + row, color);
        }
    }
}

void gfx_draw_text(int x, int y, const char* s, u32 fg, u32 bg) {
    int cx = x;
    while (*s) {
        if (*s == '\n') { y += 16; cx = x; s++; continue; }
        gfx_draw_char(cx, y, *s, fg, bg);
        cx += 8;
        s++;
    }
}

void gfx_draw_text_transparent(int x, int y, const char* s, u32 fg) {
    gfx_draw_text(x, y, s, fg, 0xFFFFFFFF);
}

int gfx_text_width(const char* s) {
    return (int)strlen(s) * 8;
}

void gfx_blit(void) {
    u32 n = pitch_px * height;
    for (u32 i = 0; i < n; i++) fb[i] = back[i];
}

/* Arrow cursor 12x19 */
static const u8 cursor_mask[19] = {
    0b10000000,
    0b11000000,
    0b11100000,
    0b11110000,
    0b11111000,
    0b11111100,
    0b11111110,
    0b11111111,
    0b11111111,
    0b11111100,
    0b11011100,
    0b10001110,
    0b00001110,
    0b00000111,
    0b00000111,
    0b00000011,
    0b00000011,
    0b00000001,
    0b00000001,
};

void gfx_draw_cursor(int x, int y) {
    for (int row = 0; row < 19; row++) {
        for (int col = 0; col < 8; col++) {
            if (cursor_mask[row] & (0x80 >> col)) {
                gfx_putpixel(x + col, y + row, 0xFFFFFFFFu);
                /* outline */
                if (col == 0 || !(cursor_mask[row] & (0x80 >> (col - 1))))
                    gfx_putpixel(x + col - 1, y + row, 0x00101010u);
            }
        }
        /* tip outline right edge */
        for (int col = 0; col < 8; col++) {
            if (cursor_mask[row] & (0x80 >> col)) {
                if (col == 7 || !(cursor_mask[row] & (0x80 >> (col + 1))))
                    gfx_putpixel(x + col + 1, y + row, 0x00101010u);
            }
        }
    }
}
