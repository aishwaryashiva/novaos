#ifndef NOVA_KEYBOARD_H
#define NOVA_KEYBOARD_H

#include "types.h"

void keyboard_init(void);
void keyboard_handler(void);
bool keyboard_has_char(void);
char keyboard_get_char(void);
bool keyboard_shift(void);

#endif
