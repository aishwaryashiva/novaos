#ifndef NOVA_GFX_H
#define NOVA_GFX_H

#include "types.h"

#define COLOR_RGB(r,g,b) ((u32)(((r)&0xFF)<<16)|(((g)&0xFF)<<8)|((b)&0xFF))

void gfx_init(boot_info_t* info);
u32  gfx_width(void);
u32  gfx_height(void);
u32* gfx_backbuffer(void);

void gfx_clear(u32 color);
void gfx_putpixel(int x, int y, u32 color);
u32  gfx_getpixel(int x, int y);
void gfx_fillrect(int x, int y, int w, int h, u32 color);
void gfx_drawrect(int x, int y, int w, int h, u32 color);
void gfx_hline(int x, int y, int w, u32 color);
void gfx_vline(int x, int y, int h, u32 color);
void gfx_fillround(int x, int y, int w, int h, int r, u32 color);
void gfx_draw_char(int x, int y, char c, u32 fg, u32 bg);
void gfx_draw_text(int x, int y, const char* s, u32 fg, u32 bg);
void gfx_draw_text_transparent(int x, int y, const char* s, u32 fg);
int  gfx_text_width(const char* s);
void gfx_blit(void);
void gfx_draw_cursor(int x, int y);

#endif
