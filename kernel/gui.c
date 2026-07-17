#include "gui.h"
#include "gfx.h"
#include "mouse.h"
#include "keyboard.h"
#include "string.h"
#include "ports.h"
#include "idt.h"

/* Palette — deep slate + copper accent (not purple / cream / newspaper) */
#define COL_DESK_TOP    0x001A2E3A
#define COL_DESK_BOT    0x000C1820
#define COL_ACCENT      0x00D4783A
#define COL_ACCENT_DIM  0x00A85A28
#define COL_PANEL       0x00141E28
#define COL_PANEL_HI    0x001E2C38
#define COL_TITLE       0x00203040
#define COL_TITLE_FOC   0x00284050
#define COL_WIN_BG      0x00F2EEE6
#define COL_WIN_TEXT    0x00182028
#define COL_TASKBAR     0x00101820
#define COL_WHITE       0x00F8F4EC
#define COL_MUTED       0x0090A0B0
#define COL_CLOSE       0x00C04040
#define COL_BTN         0x00E8E0D4
#define COL_BTN_H       0x00D4783A

static window_t windows[MAX_WINDOWS];
static int focused = -1;
static int drag_idx = -1;
static int drag_ox = 0, drag_oy = 0;
static u32 ticks = 0;
static bool menu_open = false;

/* Paint canvas stored in high memory */
#define PAINT_W 360
#define PAINT_H 220
static u32* paint_buf = (u32*)0x400000;

static void draw_desktop_bg(void) {
    u32 w = gfx_width();
    u32 h = gfx_height() - TASKBAR_H;
    for (u32 y = 0; y < h; y++) {
        u32 t = (y * 255) / (h ? h : 1);
        u32 r = ((COL_DESK_TOP >> 16) & 0xFF) * (255 - t) / 255 + ((COL_DESK_BOT >> 16) & 0xFF) * t / 255;
        u32 g = ((COL_DESK_TOP >> 8) & 0xFF) * (255 - t) / 255 + ((COL_DESK_BOT >> 8) & 0xFF) * t / 255;
        u32 b = (COL_DESK_TOP & 0xFF) * (255 - t) / 255 + (COL_DESK_BOT & 0xFF) * t / 255;
        gfx_fillrect(0, (int)y, (int)w, 1, COLOR_RGB(r, g, b));
    }
    /* soft horizon band */
    gfx_fillrect(0, (int)h / 2 - 2, (int)w, 3, 0x002A4050);
    /* brand watermark */
    gfx_draw_text_transparent((int)w / 2 - 48, (int)h / 2 - 40, "NOVAOS", COL_MUTED);
    gfx_draw_text_transparent((int)w / 2 - 88, (int)h / 2 - 16, "from scratch", 0x00506070);
}

static void draw_desktop_icons(void) {
    const char* labels[] = {"About", "Terminal", "Paint", "Calc"};
    app_type_t types[] = {APP_ABOUT, APP_TERMINAL, APP_PAINT, APP_CALC};
    for (int i = 0; i < 4; i++) {
        int x = 32;
        int y = 32 + i * 88;
        gfx_fillround(x, y, 56, 56, 6, COL_PANEL_HI);
        gfx_drawrect(x, y, 56, 56, COL_ACCENT_DIM);
        /* simple glyph */
        if (types[i] == APP_ABOUT) {
            gfx_draw_text_transparent(x + 20, y + 20, "i", COL_ACCENT);
        } else if (types[i] == APP_TERMINAL) {
            gfx_draw_text_transparent(x + 12, y + 20, ">_", COL_ACCENT);
        } else if (types[i] == APP_PAINT) {
            gfx_fillrect(x + 14, y + 14, 28, 28, COL_ACCENT);
            gfx_fillrect(x + 18, y + 18, 20, 20, 0x0030A060);
        } else {
            gfx_draw_text_transparent(x + 16, y + 20, "12", COL_ACCENT);
        }
        gfx_draw_text_transparent(x + (56 - gfx_text_width(labels[i])) / 2, y + 62, labels[i], COL_WHITE);
    }
}

static int hit_icon(int mx, int my) {
    for (int i = 0; i < 4; i++) {
        int x = 32, y = 32 + i * 88;
        if (mx >= x && mx < x + 56 && my >= y && my < y + 56) return i;
    }
    return -1;
}

window_t* gui_create_window(const char* title, app_type_t type, int x, int y, int w, int h) {
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (!windows[i].active) {
            window_t* win = &windows[i];
            memset(win, 0, sizeof(*win));
            win->active = true;
            win->focused = true;
            win->x = x; win->y = y; win->w = w; win->h = h;
            strcpy(win->title, title);
            win->type = type;
            win->calc_new = true;
            strcpy(win->calc_display, "0");
            if (type == APP_TERMINAL) {
                strcpy(win->text, "NovaOS shell v0.1\nType help\n> ");
                win->text_len = (int)strlen(win->text);
                win->cursor = win->text_len;
            } else if (type == APP_ABOUT) {
                strcpy(win->text,
                    "NovaOS\n\n"
                    "A bare-metal GUI operating\n"
                    "system written from scratch.\n\n"
                    "Bootloader, kernel, drivers,\n"
                    "and window manager — no Linux,\n"
                    "no existing OS SDK.\n\n"
                    "Hardware: x86 + VESA LFB\n"
                    "Input: PS/2 keyboard & mouse");
            } else if (type == APP_PAINT) {
                win->paint_color = COL_ACCENT;
                for (int p = 0; p < PAINT_W * PAINT_H; p++)
                    paint_buf[p] = COL_WIN_BG;
            }
            gui_focus(i);
            return win;
        }
    }
    return NULL;
}

void gui_close_window(int index) {
    if (index < 0 || index >= MAX_WINDOWS) return;
    windows[index].active = false;
    if (focused == index) {
        focused = -1;
        for (int i = MAX_WINDOWS - 1; i >= 0; i--) {
            if (windows[i].active) { gui_focus(i); break; }
        }
    }
}

void gui_focus(int index) {
    if (index < 0 || index >= MAX_WINDOWS || !windows[index].active) return;

    /* Raise window to top of z-order (end of array) */
    window_t tmp = windows[index];
    for (int i = index; i < MAX_WINDOWS - 1; i++)
        windows[i] = windows[i + 1];
    windows[MAX_WINDOWS - 1] = tmp;

    focused = -1;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        windows[i].focused = false;
        if (windows[i].active) focused = i;
    }
    if (focused >= 0) {
        windows[focused].focused = true;
        windows[focused].minimized = false;
    }
}

static int hit_window(int mx, int my) {
    for (int i = MAX_WINDOWS - 1; i >= 0; i--) {
        window_t* w = &windows[i];
        if (!w->active || w->minimized) continue;
        if (mx >= w->x && mx < w->x + w->w && my >= w->y && my < w->y + w->h)
            return i;
    }
    return -1;
}

static void terminal_exec(window_t* win, const char* cmd) {
    char out[256];
    out[0] = 0;
    if (strcmp(cmd, "help") == 0) {
        strcpy(out, "Commands: help, clear, about, uname, echo\n");
    } else if (strcmp(cmd, "clear") == 0) {
        strcpy(win->text, "> ");
        win->text_len = 2;
        win->cursor = 2;
        return;
    } else if (strcmp(cmd, "about") == 0) {
        strcpy(out, "NovaOS — handmade kernel GUI\n");
    } else if (strcmp(cmd, "uname") == 0) {
        strcpy(out, "NovaOS 0.1 x86\n");
    } else if (cmd[0] == 'e' && cmd[1] == 'c' && cmd[2] == 'h' && cmd[3] == 'o' && cmd[4] == ' ') {
        strcpy(out, cmd + 5);
        int n = (int)strlen(out);
        out[n] = '\n'; out[n + 1] = 0;
    } else if (cmd[0] == 0) {
        /* empty */
    } else {
        strcpy(out, "Unknown command. Try help.\n");
    }
    /* append output + prompt */
    if ((int)strlen(win->text) + (int)strlen(out) + 3 < 500) {
        strcpy(win->text + win->text_len, out);
        win->text_len = (int)strlen(win->text);
        strcpy(win->text + win->text_len, "> ");
        win->text_len = (int)strlen(win->text);
        win->cursor = win->text_len;
    }
}

static void terminal_key(window_t* win, char c) {
    if (c == '\b') {
        if (win->cursor > 0 && win->text[win->cursor - 1] != '\n' &&
            !(win->cursor >= 2 && win->text[win->cursor - 2] == '>' && win->text[win->cursor - 1] == ' ')) {
            /* find line start after last prompt */
            int i = win->cursor - 1;
            while (i > 0 && win->text[i] != '\n') i--;
            int prompt = i;
            if (win->text[i] == '\n') prompt = i + 1;
            if (win->cursor > prompt + 2) {
                win->cursor--;
                win->text_len--;
                for (int j = win->cursor; j < win->text_len; j++)
                    win->text[j] = win->text[j + 1];
                win->text[win->text_len] = 0;
            }
        }
        return;
    }
    if (c == '\n') {
        /* extract command after last "> " */
        int i = win->text_len - 1;
        while (i >= 0 && !(win->text[i] == '>' && i + 1 < win->text_len && win->text[i + 1] == ' '))
            i--;
        char cmd[128];
        int n = 0;
        if (i >= 0) {
            int s = i + 2;
            while (s < win->text_len && n < 127) cmd[n++] = win->text[s++];
        }
        cmd[n] = 0;
        if (win->text_len < 510) {
            win->text[win->text_len++] = '\n';
            win->text[win->text_len] = 0;
            win->cursor = win->text_len;
        }
        terminal_exec(win, cmd);
        return;
    }
    if (c >= 32 && c < 127 && win->text_len < 500) {
        win->text[win->text_len++] = c;
        win->text[win->text_len] = 0;
        win->cursor = win->text_len;
    }
}

static i32 calc_parse(const char* s) {
    i32 v = 0;
    int neg = 0;
    if (*s == '-') { neg = 1; s++; }
    while (*s >= '0' && *s <= '9') { v = v * 10 + (*s - '0'); s++; }
    return neg ? -v : v;
}

static void calc_input(window_t* win, char key) {
    if (key >= '0' && key <= '9') {
        if (win->calc_new || (win->calc_display[0] == '0' && win->calc_display[1] == 0)) {
            win->calc_display[0] = key;
            win->calc_display[1] = 0;
            win->calc_new = false;
        } else if ((int)strlen(win->calc_display) < 12) {
            int n = (int)strlen(win->calc_display);
            win->calc_display[n] = key;
            win->calc_display[n + 1] = 0;
        }
    } else if (key == 'C') {
        strcpy(win->calc_display, "0");
        win->calc_acc = 0;
        win->calc_op = 0;
        win->calc_new = true;
    } else if (key == '+' || key == '-' || key == '*' || key == '/') {
        i32 v = calc_parse(win->calc_display);
        if (win->calc_op && !win->calc_new) {
            if (win->calc_op == '+') win->calc_acc += v;
            else if (win->calc_op == '-') win->calc_acc -= v;
            else if (win->calc_op == '*') win->calc_acc *= v;
            else if (win->calc_op == '/' && v != 0) win->calc_acc /= v;
        } else {
            win->calc_acc = v;
        }
        win->calc_op = key;
        win->calc_new = true;
        itoa((int)win->calc_acc, win->calc_display, 10);
    } else if (key == '=') {
        char op = win->calc_op ? win->calc_op : '+';
        calc_input(win, op);
        win->calc_op = 0;
        win->calc_new = true;
    }
}

static void draw_window(window_t* win) {
    if (!win->active || win->minimized) return;
    int x = win->x, y = win->y, w = win->w, h = win->h;
    u32 title_col = win->focused ? COL_TITLE_FOC : COL_TITLE;

    /* shadow */
    gfx_fillrect(x + 4, y + 4, w, h, 0x00080810);

    gfx_fillround(x, y, w, h, 4, COL_WIN_BG);
    gfx_fillrect(x, y, w, TITLE_H, title_col);
    gfx_draw_text_transparent(x + 10, y + 6, win->title, COL_WHITE);

    /* close button */
    gfx_fillround(x + w - 24, y + 6, 16, 16, 3, COL_CLOSE);
    gfx_draw_text_transparent(x + w - 20, y + 6, "x", COL_WHITE);

    int cx = x + 8, cy = y + TITLE_H + 8;
    int cw = w - 16, ch = h - TITLE_H - 16;

    if (win->type == APP_ABOUT) {
        gfx_draw_text(cx, cy, win->text, COL_WIN_TEXT, COL_WIN_BG);
    } else if (win->type == APP_TERMINAL) {
        gfx_fillrect(x + 1, y + TITLE_H, w - 2, h - TITLE_H - 1, 0x00101820);
        gfx_draw_text(cx, cy, win->text, 0x00A8D8A0, 0x00101820);
    } else if (win->type == APP_PAINT) {
        /* palette */
        u32 colors[] = {0x00182028, COL_ACCENT, 0x00C04040, 0x0030A060, 0x003070C0, 0x00F2EEE6};
        for (int i = 0; i < 6; i++) {
            gfx_fillrect(cx + i * 28, cy, 24, 24, colors[i]);
            if (win->paint_color == colors[i])
                gfx_drawrect(cx + i * 28 - 1, cy - 1, 26, 26, COL_WIN_TEXT);
        }
        int px = cx, py = cy + 36;
        for (int row = 0; row < PAINT_H && row < ch - 36; row++) {
            for (int col = 0; col < PAINT_W && col < cw; col++) {
                gfx_putpixel(px + col, py + row, paint_buf[row * PAINT_W + col]);
            }
        }
        gfx_drawrect(px, py, PAINT_W, PAINT_H, COL_WIN_TEXT);
    } else if (win->type == APP_CALC) {
        gfx_fillrect(cx, cy, cw, 40, COL_PANEL);
        int tw = gfx_text_width(win->calc_display);
        gfx_draw_text_transparent(cx + cw - tw - 8, cy + 12, win->calc_display, COL_WHITE);
        const char* keys = "789/456*123-C0=+" ;
        /* layout 4x4 */
        char grid[16] = {'7','8','9','/','4','5','6','*','1','2','3','-','C','0','=','+'};
        for (int i = 0; i < 16; i++) {
            int bx = cx + (i % 4) * (cw / 4 + 2);
            int by = cy + 56 + (i / 4) * 40;
            int bw = cw / 4 - 4;
            gfx_fillround(bx, by, bw, 34, 4, COL_BTN);
            char s[2] = {grid[i], 0};
            gfx_draw_text_transparent(bx + bw / 2 - 4, by + 9, s, COL_WIN_TEXT);
        }
        (void)keys;
    }
}

static void draw_taskbar(void) {
    int w = (int)gfx_width();
    int h = (int)gfx_height();
    int ty = h - TASKBAR_H;
    gfx_fillrect(0, ty, w, TASKBAR_H, COL_TASKBAR);
    gfx_hline(0, ty, w, COL_ACCENT_DIM);

    /* Start / brand */
    gfx_fillround(8, ty + 6, 88, 28, 4, menu_open ? COL_ACCENT : COL_PANEL_HI);
    gfx_draw_text_transparent(20, ty + 12, "NovaOS", COL_WHITE);

    /* window buttons */
    int bx = 108;
    for (int i = 0; i < MAX_WINDOWS; i++) {
        if (!windows[i].active) continue;
        u32 col = windows[i].focused ? COL_ACCENT_DIM : COL_PANEL;
        gfx_fillround(bx, ty + 8, 100, 24, 3, col);
        char shortt[12];
        int n = 0;
        while (windows[i].title[n] && n < 10) { shortt[n] = windows[i].title[n]; n++; }
        shortt[n] = 0;
        gfx_draw_text_transparent(bx + 8, ty + 12, shortt, COL_WHITE);
        bx += 108;
    }

    /* clock from PIT ticks (~18.2 Hz if we used PIT; we approximate) */
    char clock[16];
    u32 sec = ticks / 20;
    u32 hh = (sec / 3600) % 24;
    u32 mm = (sec / 60) % 60;
    u32 ss = sec % 60;
    clock[0] = (char)('0' + hh / 10);
    clock[1] = (char)('0' + hh % 10);
    clock[2] = ':';
    clock[3] = (char)('0' + mm / 10);
    clock[4] = (char)('0' + mm % 10);
    clock[5] = ':';
    clock[6] = (char)('0' + ss / 10);
    clock[7] = (char)('0' + ss % 10);
    clock[8] = 0;
    gfx_draw_text_transparent(w - 80, ty + 12, clock, COL_MUTED);
}

static void draw_menu(void) {
    if (!menu_open) return;
    int h = (int)gfx_height();
    int ty = h - TASKBAR_H;
    gfx_fillround(8, ty - 180, 160, 172, 4, COL_PANEL);
    gfx_drawrect(8, ty - 180, 160, 172, COL_ACCENT_DIM);
    const char* items[] = {"About NovaOS", "Terminal", "Paint", "Calculator", "Close Menu"};
    for (int i = 0; i < 5; i++) {
        gfx_draw_text_transparent(24, ty - 164 + i * 30, items[i], COL_WHITE);
    }
}

static void open_app(app_type_t t) {
    switch (t) {
        case APP_ABOUT:
            gui_create_window("About NovaOS", APP_ABOUT, 220, 80, 360, 300);
            break;
        case APP_TERMINAL:
            gui_create_window("Terminal", APP_TERMINAL, 180, 100, 520, 340);
            break;
        case APP_PAINT:
            gui_create_window("Paint", APP_PAINT, 200, 60, 400, 340);
            break;
        case APP_CALC:
            gui_create_window("Calculator", APP_CALC, 340, 120, 280, 340);
            break;
        default: break;
    }
}

static void handle_click(int mx, int my) {
    int h = (int)gfx_height();
    int ty = h - TASKBAR_H;

    /* menu items */
    if (menu_open && mx >= 8 && mx < 168 && my >= ty - 180 && my < ty) {
        int item = (my - (ty - 164)) / 30;
        menu_open = false;
        if (item == 0) open_app(APP_ABOUT);
        else if (item == 1) open_app(APP_TERMINAL);
        else if (item == 2) open_app(APP_PAINT);
        else if (item == 3) open_app(APP_CALC);
        return;
    }

    /* start button */
    if (mx >= 8 && mx < 96 && my >= ty + 6 && my < ty + 34) {
        menu_open = !menu_open;
        return;
    }

    /* taskbar window buttons */
    if (my >= ty) {
        int bx = 108;
        for (int i = 0; i < MAX_WINDOWS; i++) {
            if (!windows[i].active) continue;
            if (mx >= bx && mx < bx + 100) {
                if (windows[i].minimized || !windows[i].focused)
                    gui_focus(i);
                else
                    windows[i].minimized = true;
                menu_open = false;
                return;
            }
            bx += 108;
        }
        menu_open = false;
        return;
    }

    menu_open = false;

    int wi = hit_window(mx, my);
    if (wi >= 0) {
        window_t* win = &windows[wi];
        gui_focus(wi);
        wi = focused;
        win = &windows[wi];

        /* close */
        if (mx >= win->x + win->w - 24 && mx < win->x + win->w - 8 &&
            my >= win->y + 6 && my < win->y + 22) {
            gui_close_window(wi);
            return;
        }
        /* title drag */
        if (my < win->y + TITLE_H) {
            drag_idx = wi;
            drag_ox = mx - win->x;
            drag_oy = my - win->y;
            return;
        }

        if (win->type == APP_PAINT) {
            int cx = win->x + 8, cy = win->y + TITLE_H + 8;
            u32 colors[] = {0x00182028, COL_ACCENT, 0x00C04040, 0x0030A060, 0x003070C0, 0x00F2EEE6};
            for (int i = 0; i < 6; i++) {
                if (mx >= cx + i * 28 && mx < cx + i * 28 + 24 && my >= cy && my < cy + 24)
                    win->paint_color = colors[i];
            }
            int px = cx, py = cy + 36;
            if (mx >= px && mx < px + PAINT_W && my >= py && my < py + PAINT_H) {
                int col = mx - px, row = my - py;
                for (int dy = -2; dy <= 2; dy++)
                    for (int dx = -2; dx <= 2; dx++) {
                        int rr = row + dy, cc = col + dx;
                        if (rr >= 0 && rr < PAINT_H && cc >= 0 && cc < PAINT_W)
                            paint_buf[rr * PAINT_W + cc] = win->paint_color;
                    }
            }
        } else if (win->type == APP_CALC) {
            int cx = win->x + 8, cy = win->y + TITLE_H + 8;
            int cw = win->w - 16;
            char grid[16] = {'7','8','9','/','4','5','6','*','1','2','3','-','C','0','=','+'};
            for (int i = 0; i < 16; i++) {
                int bx = cx + (i % 4) * (cw / 4 + 2);
                int by = cy + 56 + (i / 4) * 40;
                int bw = cw / 4 - 4;
                if (mx >= bx && mx < bx + bw && my >= by && my < by + 34)
                    calc_input(win, grid[i]);
            }
        }
        return;
    }

    int icon = hit_icon(mx, my);
    if (icon == 0) open_app(APP_ABOUT);
    else if (icon == 1) open_app(APP_TERMINAL);
    else if (icon == 2) open_app(APP_PAINT);
    else if (icon == 3) open_app(APP_CALC);
}

static void pit_handler(void) {
    ticks++;
}

void gui_init(void) {
    memset(windows, 0, sizeof(windows));
    focused = -1;
    irq_install_handler(0, pit_handler);

    /* Program PIT to ~20Hz */
    u32 div = 1193180 / 20;
    outb(0x43, 0x36);
    outb(0x40, (u8)(div & 0xFF));
    outb(0x40, (u8)((div >> 8) & 0xFF));

    open_app(APP_ABOUT);
}

void gui_run(void) {
    while (1) {
        mouse_poll_buttons();
        int mx = mouse_x();
        int my = mouse_y();

        if (mouse_left_pressed())
            handle_click(mx, my);

        if (mouse_left() && drag_idx >= 0 && windows[drag_idx].active) {
            windows[drag_idx].x = mx - drag_ox;
            windows[drag_idx].y = my - drag_oy;
            if (windows[drag_idx].x < 0) windows[drag_idx].x = 0;
            if (windows[drag_idx].y < 0) windows[drag_idx].y = 0;
        }
        if (mouse_left_released())
            drag_idx = -1;

        /* paint while held */
        if (mouse_left() && focused >= 0 && windows[focused].active &&
            windows[focused].type == APP_PAINT && drag_idx < 0) {
            window_t* win = &windows[focused];
            int cx = win->x + 8, cy = win->y + TITLE_H + 8;
            int px = cx, py = cy + 36;
            if (mx >= px && mx < px + PAINT_W && my >= py && my < py + PAINT_H) {
                int col = mx - px, row = my - py;
                for (int dy = -2; dy <= 2; dy++)
                    for (int dx = -2; dx <= 2; dx++) {
                        int rr = row + dy, cc = col + dx;
                        if (rr >= 0 && rr < PAINT_H && cc >= 0 && cc < PAINT_W)
                            paint_buf[rr * PAINT_W + cc] = win->paint_color;
                    }
            }
        }

        while (keyboard_has_char()) {
            char c = keyboard_get_char();
            if (focused >= 0 && windows[focused].active && windows[focused].type == APP_TERMINAL)
                terminal_key(&windows[focused], c);
        }

        draw_desktop_bg();
        draw_desktop_icons();
        for (int i = 0; i < MAX_WINDOWS; i++)
            draw_window(&windows[i]);
        draw_taskbar();
        draw_menu();
        gfx_draw_cursor(mx, my);
        gfx_blit();

        __asm__ volatile ("hlt");
    }
}
