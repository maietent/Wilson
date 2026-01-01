#include "desktop.h"
#include "framebuffer.h"
#include "memory.h"
#include "font.h"
#include "alloc.h"
#include "cursor.h"
#include "string.h"

static uint32_t cursor_backing[CURSOR_WIDTH * CURSOR_HEIGHT];
static bool cursor_visible = false;
static int last_x = 0;
static int last_y = 0;

uint32_t* terminal_buffer = NULL;
uint32_t* desktop_buffer = NULL;
size_t buffer_size = 0;
uint32_t* limine_fb = NULL;

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
    uint32_t *fb = get_fb();
    size_t pitch = get_fb_pitch();

    d_restore_cursor(fb, pitch);
    d_draw_cursor(fb, pitch, cursor_x, cursor_y);
}

void d_handle_keyboard(char key_char, uint8_t scancode)
{

}
