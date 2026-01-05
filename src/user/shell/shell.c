#include "shell.h"
#include "tty.h"
#include "desktop.h"
#include "framebuffer.h"
#include "font.h"
#include "alloc.h"
#include "cursor.h"
#include "cpu_utils.h"
#include "memory.h"
#include "terminal.h"
#include "klog.h"
#include "input.h"

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

    input_push_key(key_char, scancode);
}

void s_handle_mouse(int cursor_x, int cursor_y)
{
    input_push_mouse_move(cursor_x, cursor_y);
}

void shell_run(void)
{
    limine_fb = get_fb();
    size_t s_width = get_fb_width();
    size_t s_height = get_fb_height();
    buffer_size = s_width * s_height;

    terminal_buffer = (uint32_t *)kmalloc(buffer_size * sizeof(uint32_t));
    desktop_buffer = (uint32_t *)kmalloc(buffer_size * sizeof(uint32_t));

    d_init_double_buffering();

    memset(terminal_buffer, 0, buffer_size * sizeof(uint32_t));
    memset(desktop_buffer, 0x000000, buffer_size * sizeof(uint32_t));

    //t_clear();
    t_printf("\n\n===================================\n");
    t_printf("=           Wilson v0.01          =\n");
    t_printf("=    F1 for TTY, F2 for Desktop   =\n");
    t_printf("=  \"help\" for a list of commands  =\n");
    t_printf("===================================\n");
    //t_printf("$ ");
    t_draw_prompt();

    memcpy(terminal_buffer, limine_fb, buffer_size * sizeof(uint32_t));
    memcpy(limine_fb, desktop_buffer, buffer_size * sizeof(uint32_t));
    draw_string(limine_fb, get_fb_pitch(), (get_fb_width() / 2) - (FONT_WIDTH * 4), 1, "Desktop", 0xFFFFFF);
    memcpy(desktop_buffer, limine_fb, buffer_size * sizeof(uint32_t));

    memcpy(limine_fb, terminal_buffer, buffer_size * sizeof(uint32_t));
    current_mode = 0;

    while (1)
    {
        input_event_t ev;

        while (input_pop_event(&ev))
        {
            switch (ev.type)
            {
                case INPUT_KEY:
                    if (current_mode)
                        d_handle_keyboard(ev.key.key_char, ev.key.scancode);
                    else
                        t_handle_keyboard(ev.key.key_char, ev.key.scancode);
                    break;

                case INPUT_MOUSE_MOVE:
                    if (current_mode)
                        d_handle_mouse(ev.mouse.x, ev.mouse.y);
                    break;
            }
        }

        if (current_mode)
            d_tick();
        else
            t_tick();
    }
    CU_halt();
}
