#pragma once

#include "std.h"

typedef void (*keyboard_handler_fn)(char key_char, uint8_t scancode);

void keyboard_handler_c(void);
void keyboard_init(keyboard_handler_fn handler);
