#include "desktop.h"
#include "framebuffer.h"
#include "memory.h"
#include "font.h"
#include "alloc.h"
#include "cursor.h"
#include "string.h"

static uint32_t cursor_backing[CURSOR_WIDTH * CURSOR_HEIGHT];

uint32_t* terminal_buffer = NULL;
uint32_t* desktop_buffer = NULL;
size_t buffer_size = 0;
uint32_t* limine_fb = NULL;

bool cursor_visible = false;
int last_x = 0;
int last_y = 0;

void d_init_double_buffering(void)
{
    limine_fb = get_fb();
    size_t pitch = get_fb_pitch();
    size_t height = get_fb_height();

    buffer_size = pitch * height;

    desktop_buffer = kmalloc(buffer_size * sizeof(uint32_t));
    terminal_buffer = kmalloc(buffer_size * sizeof(uint32_t));

    memset(desktop_buffer, 0, buffer_size * sizeof(uint32_t));
    memset(terminal_buffer, 0, buffer_size * sizeof(uint32_t));
}

void d_resize_buffers(size_t new_pitch, size_t new_height)
{
    size_t new_size = new_pitch * new_height;

    desktop_buffer = krealloc(desktop_buffer, new_size * sizeof(uint32_t));
    terminal_buffer = krealloc(terminal_buffer, new_size * sizeof(uint32_t));

    buffer_size = new_size;
}

void d_shutdown_desktop(void)
{
    if (desktop_buffer) kfree(desktop_buffer);
    if (terminal_buffer) kfree(terminal_buffer);

    desktop_buffer = NULL;
    terminal_buffer = NULL;
}

void d_switch_to_desktop(void)
{
    memcpy(terminal_buffer, limine_fb, buffer_size * sizeof(uint32_t));
    memcpy(limine_fb, desktop_buffer, buffer_size * sizeof(uint32_t));
}

void d_switch_to_terminal(void)
{
    memcpy(desktop_buffer, limine_fb, buffer_size * sizeof(uint32_t));
    memcpy(limine_fb, terminal_buffer, buffer_size * sizeof(uint32_t));
}

void d_switch_buffer(int wanted_buf)
{
    if (wanted_buf)
        d_switch_to_desktop();
    else
        d_switch_to_terminal();
}

void d_restore_cursor(uint32_t *fb, size_t pitch)
{
    if (!cursor_visible)
        return;

    for (int cy = 0; cy < CURSOR_HEIGHT; cy++)
        for (int cx = 0; cx < CURSOR_WIDTH; cx++)
            fb[(last_y + cy) * pitch + (last_x + cx)] = cursor_backing[cy * CURSOR_WIDTH + cx];

    cursor_visible = false;
}

void d_draw_cursor(uint32_t *fb, size_t pitch, int x, int y)
{
    for (int cy = 0; cy < CURSOR_HEIGHT; cy++)
        for (int cx = 0; cx < CURSOR_WIDTH; cx++)
            cursor_backing[cy * CURSOR_WIDTH + cx] = fb[(y + cy) * pitch + (x + cx)];

    for (int cy = 0; cy < CURSOR_HEIGHT; cy++)
        for (int cx = 0; cx < CURSOR_WIDTH; cx++)
        {
            uint32_t px = cursor_data[cy * CURSOR_WIDTH + cx];
            if ((px & 0x00FFFFFF) == 0) continue;
            fb[(y + cy) * pitch + (x + cx)] = px;
        }

    last_x = x;
    last_y = y;
    cursor_visible = true;
}

void d_handle_mouse(int cursor_x, int cursor_y)
{
    size_t pitch = get_fb_pitch();

    d_restore_cursor(desktop_buffer, pitch);
    d_draw_cursor(desktop_buffer, pitch, cursor_x, cursor_y);
}

void d_handle_keyboard(char key_char, uint8_t scancode)
{

}

void d_tick(void)
{
    if (!desktop_buffer || !limine_fb)
        return;

    size_t pitch = get_fb_pitch();

    memcpy(limine_fb, desktop_buffer, buffer_size * sizeof(uint32_t));
}
