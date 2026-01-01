#pragma once

#include "std.h"

void t_handle_keyboard(char key_char, uint8_t scancode);
void t_handle_backspace(void);
void t_handle_enter(void);
void t_draw_prompt(void);
void t_command_handler(void);

extern char t_cmd_buf[512];
extern size_t t_cmd_len;
