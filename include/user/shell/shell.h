#pragma once

#include "std.h"

void s_handle_mouse(int cursor_x, int cursor_y);
void s_handle_keyboard(char key_char, uint8_t scancode);
void shell_run(void);

extern int current_mode;
