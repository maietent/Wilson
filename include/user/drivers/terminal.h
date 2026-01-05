#pragma once

#include "std.h"

void t_reset_cursor();
void t_clear();
void t_set_color(uint32_t color);
void t_set_fg_color(uint32_t color);
void t_set_bg_color(uint32_t color);
void t_set_cell_bg(size_t x, size_t y, uint32_t color);
void t_draw_header(const char *text, uint32_t fg, uint32_t bg);
void t_scroll();
void t_newline();
void t_backspace();
void t_putchar(char c);
void t_drawstring(const char* str);
void t_printf(const char* str, ...);
bool init_terminal();
