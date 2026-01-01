#pragma once

#include "std.h"

void d_switch_to_desktop(void);
void d_switch_to_terminal(void);
void d_switch_buffer(int wanted_buf);
void d_handle_mouse(int cursor_x, int cursor_y);
void d_draw_cursor(uint32_t *fb, size_t pitch, int x, int y);
void d_restore_cursor(uint32_t *fb, size_t pitch);

extern uint32_t* terminal_buffer;
extern uint32_t* desktop_buffer;
extern size_t buffer_size;
extern uint32_t* limine_fb;
