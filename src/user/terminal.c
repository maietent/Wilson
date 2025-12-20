#include "terminal.h"
#include "font.h"
#include "framebuffer.h"
#include "printf.h"

static uint32_t *t_fb;
static size_t t_fb_pitch;
static size_t t_fb_width = 0;
static size_t t_fb_height = 0;
static uint32_t t_color = 0XFFFFFF;

static size_t t_cursor_x = 0;
static size_t t_cursor_y = 0;

#define TERMINAL_WIDTH (t_fb_width / FONT_WIDTH)
#define TERMINAL_HEIGHT (t_fb_height / FONT_HEIGHT)

void t_reset_cursor()
{
    t_cursor_x = 0;
    t_cursor_y = 0;
}

void t_clear()
{
    fb_clear();
    t_reset_cursor();
}

void t_set_color(uint32_t color)
{
    t_color = color;
}

void t_newline()
{
    t_cursor_x = 0;
    t_cursor_y++;
    // todo: scroll
}

void t_putchar(char c)
{
    if (c == '\n')
    {
        t_newline();
        return;
    }

    draw_char(t_fb, t_fb_pitch, t_cursor_x * FONT_WIDTH, t_cursor_y * FONT_HEIGHT, c, t_color);

    t_cursor_x++;
    if (t_cursor_x >= TERMINAL_WIDTH)
        t_newline();
}

void t_drawstring(const char* str)
{
    while(*str)
    {
        t_putchar(*str);
        str++;
    }
}

void t_printf(const char* str, ...)
{
    char buf[512];
    va_list args;

    va_start(args, str);
    vsnprintf(buf, sizeof(buf), str, args);
    va_end(args);

    t_drawstring(buf);
}

bool init_terminal()
{
    t_fb = get_fb();
    t_fb_pitch = get_fb_pitch();
    t_fb_width = get_fb_width();
    t_fb_height = get_fb_height();

    t_clear();
    return true;
}
