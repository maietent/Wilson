#include "shell.h"
#include "t_shell.h"
#include "desktop.h"
#include "framebuffer.h"
#include "font.h"
#include "alloc.h"
#include "cursor.h"
#include "cpu_utils.h"
#include "memory.h"
#include "terminal.h"

int current_mode = 0;

void switch_buffer(int wanted_buf)
{
    if (wanted_buf == current_mode) return;

    if (wanted_buf)
    {
        d_switch_to_desktop();
        current_mode = 1;
    }
    else
    {
        d_switch_to_terminal();
        current_mode = 0;
    }
}

void s_handle_keyboard(char key_char, uint8_t scancode)
{
    if (scancode == 0x3b) { // f1
        switch_buffer(0);
        return;
    }
    if (scancode == 0x3c) { // f2
        switch_buffer(1);
        return;
    }

    if (current_mode)
    {

    }
    else
    {
        t_handle_keyboard(key_char, scancode);
    }
}

void s_handle_mouse(int cursor_x, int cursor_y)
{
    if (!current_mode)
        return;

    d_handle_mouse(cursor_x, cursor_y);
}

void shell_run(void)
{
    limine_fb = get_fb();
    size_t s_width = get_fb_width();
    size_t s_height = get_fb_height();
    buffer_size = s_width * s_height;

    terminal_buffer = (uint32_t *)kmalloc(buffer_size * sizeof(uint32_t));
    desktop_buffer = (uint32_t *)kmalloc(buffer_size * sizeof(uint32_t));

    memset(terminal_buffer, 0, buffer_size * sizeof(uint32_t));
    memset(desktop_buffer, 0x000000, buffer_size * sizeof(uint32_t));

    t_clear();
    t_draw_header("Wilson - v0.1", 0x000000, 0xFFFFFF);
    t_printf("> ");

    memcpy(terminal_buffer, limine_fb, buffer_size * sizeof(uint32_t));
    memcpy(limine_fb, desktop_buffer, buffer_size * sizeof(uint32_t));
    draw_string(limine_fb, get_fb_pitch(), (get_fb_width() / 2) - (FONT_WIDTH * 4), 1, "Desktop", 0xFFFFFF);
    memcpy(desktop_buffer, limine_fb, buffer_size * sizeof(uint32_t));

    memcpy(limine_fb, terminal_buffer, buffer_size * sizeof(uint32_t));
    current_mode = 0;

    CU_halt();
}
