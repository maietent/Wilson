#pragma once

#include "std.h"

void s_handle_keyboard(char key_char, uint8_t scancode);
void s_handle_mouse(int cursor_x, int cursor_y);
void shell_run(void);
