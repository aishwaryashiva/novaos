#ifndef NOVA_GUI_H
#define NOVA_GUI_H

#include "types.h"

#define MAX_WINDOWS 8
#define TITLE_H     28
#define TASKBAR_H   40

typedef enum {
    APP_NONE = 0,
    APP_ABOUT,
    APP_TERMINAL,
    APP_PAINT,
    APP_CALC
} app_type_t;

typedef struct {
    bool active;
    bool focused;
    bool minimized;
    int x, y, w, h;
    char title[32];
    app_type_t type;
    /* app-specific state */
    char text[512];
    int text_len;
    int cursor;
    u32 paint_color;
    int paint_dirty;
    char calc_display[32];
    i32 calc_acc;
    char calc_op;
    bool calc_new;
} window_t;

void gui_init(void);
void gui_run(void);
window_t* gui_create_window(const char* title, app_type_t type, int x, int y, int w, int h);
void gui_close_window(int index);
void gui_focus(int index);

#endif
