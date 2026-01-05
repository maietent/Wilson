#pragma once
#include "std.h"

bool init_terminal(void);
void t_clear(void);
void t_scroll(void);
void t_newline(void);
void t_backspace(void);
void t_putchar(char c);
void t_drawstring(const char* str);
void t_printf(const char* fmt, ...);
void t_draw_header(const char *text, uint32_t fg, uint32_t bg);
void t_reset_cursor(void);
void t_set_fg_color(uint32_t color);
void t_set_bg_color(uint32_t color);
void t_set_color(uint32_t color);
void t_set_cell_bg(size_t x, size_t y, uint32_t color);

uint32_t t_get_fg_color(void);
uint32_t t_get_bg_color(void);
