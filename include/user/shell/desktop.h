#pragma once

#include "std.h"

extern uint32_t* terminal_buffer;
extern uint32_t* desktop_buffer;
extern size_t buffer_size;
extern uint32_t* limine_fb;

extern bool cursor_visible;
extern int last_x;
extern int last_y;

void d_init_double_buffering(void);

void d_switch_to_desktop(void);
void d_switch_to_terminal(void);
void d_switch_buffer(int wanted_buf);

void d_handle_mouse(int cursor_x, int cursor_y);
void d_draw_cursor(uint32_t *fb, size_t pitch, int x, int y);
void d_restore_cursor(uint32_t *fb, size_t pitch);

void d_handle_keyboard(char key_char, uint8_t scancode);

void d_tick(void);

void d_resize_buffers(size_t new_pitch, size_t new_height);

void d_shutdown_desktop(void);
