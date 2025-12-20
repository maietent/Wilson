#pragma once

#include "std.h"

void t_reset_cursor();
void t_clear();
void t_set_color(uint32_t color);
void t_newline();
void t_putchar(char c);
void t_drawstring(const char* str);
bool init_terminal();
