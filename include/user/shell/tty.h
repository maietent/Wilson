#pragma once

#include "std.h"

#define TTY_CMD_BUF_SIZE 512
#define TTY_HISTORY_SIZE 10

extern char t_cmd_buf[TTY_CMD_BUF_SIZE];
extern size_t t_cmd_len;

typedef void (*t_command_func_t)(void);

typedef struct {
    const char* name;
    const char* description;
    t_command_func_t function;
} t_command_t;

extern t_command_t t_commands[];
extern size_t t_command_count;

extern char t_history[TTY_HISTORY_SIZE][TTY_CMD_BUF_SIZE];
extern int t_history_count;
extern int t_history_index;

extern char* t_command_args;

void t_handle_keyboard(char key_char, uint8_t scancode);
void t_handle_backspace(void);
void t_handle_enter(void);
void t_draw_prompt(void);
void t_command_handler(void);

void t_cmd_help(void);
void t_cmd_echo(void);
void t_cmd_clear(void);
void t_cmd_reboot(void);
void t_cmd_rng(void);
void t_cmd_info(void);
void t_cmd_sleep(void);
void t_cmd_getticks(void);
void t_cmd_klog(void);
void t_cmd_syscallc(void);
void t_cmd_ata_read(void);

void t_tick();
