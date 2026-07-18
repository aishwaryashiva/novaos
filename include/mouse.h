#ifndef NOVA_MOUSE_H
#define NOVA_MOUSE_H

#include "types.h"

void mouse_init(void);
void mouse_handler(void);
int  mouse_x(void);
int  mouse_y(void);
bool mouse_left(void);
bool mouse_right(void);
bool mouse_left_pressed(void);
bool mouse_left_released(void);
void mouse_poll_buttons(void);

#endif
