#include "terminal.h"
#include "font.h"
#include "framebuffer.h"
#include "printf.h"
#include "alloc.h"
#include "string.h"

#define TERMINAL_RESERVED_ROWS 1

static uint32_t *t_fb;
static size_t t_fb_pitch;
static size_t t_fb_width;
static size_t t_fb_height;

static uint32_t t_fg_color = 0xFFFFFF;
static uint32_t t_bg_color = 0x000000;

static size_t t_cursor_x = 0;
static size_t t_cursor_y = TERMINAL_RESERVED_ROWS;

static uint32_t *t_cell_bg;

static bool t_allow_reserved = false;

#define TERMINAL_WIDTH  (t_fb_width / FONT_WIDTH)
#define TERMINAL_HEIGHT (t_fb_height / FONT_HEIGHT)
#define CELL_INDEX(x, y) ((y) * TERMINAL_WIDTH + (x))

static void draw_cell_bg(size_t cx, size_t cy, uint32_t color)
{
    size_t px = cx * FONT_WIDTH;
    size_t py = cy * FONT_HEIGHT;

    for (size_t y = 0; y < FONT_HEIGHT; y++)
        for (size_t x = 0; x < FONT_WIDTH; x++)
            t_fb[(py + y) * t_fb_pitch + (px + x)] = color;
}

void t_reset_cursor()
{
    t_cursor_x = 0;
    t_cursor_y = TERMINAL_RESERVED_ROWS;
}

void t_set_fg_color(uint32_t color)
{
    t_fg_color = color;
}

void t_set_bg_color(uint32_t color)
{
    t_bg_color = color;
}

void t_set_color(uint32_t color)
{
    t_fg_color = color;
}

void t_set_cell_bg(size_t x, size_t y, uint32_t color)
{
    if (x < TERMINAL_WIDTH && y < TERMINAL_HEIGHT)
        t_cell_bg[CELL_INDEX(x, y)] = color;
}

void t_clear()
{
    for (size_t y = 0; y < TERMINAL_HEIGHT; y++)
    {
        for (size_t x = 0; x < TERMINAL_WIDTH; x++)
        {
            t_cell_bg[CELL_INDEX(x, y)] = t_bg_color;
            draw_cell_bg(x, y, t_bg_color);
        }
    }

    t_reset_cursor();
}


void t_scroll()
{
    size_t start_px = TERMINAL_RESERVED_ROWS * FONT_HEIGHT;
    size_t end_px   = (TERMINAL_HEIGHT - 1) * FONT_HEIGHT;

    for (size_t y = start_px; y < end_px; y++)
    {
        for (size_t x = 0; x < t_fb_width; x++)
        {
            t_fb[y * t_fb_pitch + x] =
                t_fb[(y + FONT_HEIGHT) * t_fb_pitch + x];
        }
    }

    for (size_t y = TERMINAL_RESERVED_ROWS; y < TERMINAL_HEIGHT - 1; y++)
    {
        for (size_t x = 0; x < TERMINAL_WIDTH; x++)
        {
            t_cell_bg[CELL_INDEX(x, y)] =
                t_cell_bg[CELL_INDEX(x, y + 1)];
        }
    }

    for (size_t x = 0; x < TERMINAL_WIDTH; x++)
    {
        t_cell_bg[CELL_INDEX(x, TERMINAL_HEIGHT - 1)] = t_bg_color;
        draw_cell_bg(x, TERMINAL_HEIGHT - 1, t_bg_color);
    }

    if (t_cursor_y > TERMINAL_RESERVED_ROWS)
        t_cursor_y--;
}

void t_newline()
{
    t_cursor_x = 0;
    t_cursor_y++;

    if (t_cursor_y >= TERMINAL_HEIGHT)
        t_scroll();
}

void t_backspace()
{
    if (t_cursor_x == 0 && t_cursor_y == TERMINAL_RESERVED_ROWS)
        return;

    if (t_cursor_x == 0)
    {
        t_cursor_y--;
        t_cursor_x = TERMINAL_WIDTH - 1;
    }
    else
    {
        t_cursor_x--;
    }

    uint32_t bg = t_cell_bg[CELL_INDEX(t_cursor_x, t_cursor_y)];
    draw_cell_bg(t_cursor_x, t_cursor_y, bg);
}

void t_putchar(char c)
{
    if (c == '\n')
    {
        t_newline();
        return;
    }

    if (c == '\b')
    {
        t_backspace();
        return;
    }

    if (!t_allow_reserved && t_cursor_y < TERMINAL_RESERVED_ROWS)
        t_cursor_y = TERMINAL_RESERVED_ROWS;

    uint32_t bg = t_cell_bg[CELL_INDEX(t_cursor_x, t_cursor_y)];

    draw_cell_bg(t_cursor_x, t_cursor_y, bg);

    draw_char(
        t_fb,
        t_fb_pitch,
        t_cursor_x * FONT_WIDTH,
        t_cursor_y * FONT_HEIGHT,
        c,
        t_fg_color
    );

    t_cursor_x++;
    if (t_cursor_x >= TERMINAL_WIDTH)
        t_newline();
}

void t_drawstring(const char* str)
{
    while (*str)
        t_putchar(*str++);
}

void t_printf(const char* fmt, ...)
{
    char buf[512];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    t_drawstring(buf);
}

void t_draw_header(const char *text, uint32_t fg, uint32_t bg)
{
    size_t len = strlen(text);
    size_t start_x = 0;

    if (len < TERMINAL_WIDTH)
        start_x = (TERMINAL_WIDTH - len) / 2;

    for (size_t x = 0; x < TERMINAL_WIDTH; x++)
    {
        t_cell_bg[CELL_INDEX(x, 0)] = bg;
        draw_cell_bg(x, 0, bg);
    }

    size_t old_x = t_cursor_x;
    size_t old_y = t_cursor_y;
    uint32_t old_fg = t_fg_color;
    bool old_allow = t_allow_reserved;

    t_allow_reserved = true;

    t_cursor_x = start_x;
    t_cursor_y = 0;
    t_fg_color = fg;

    t_drawstring(text);

    t_allow_reserved = old_allow;
    t_cursor_x = old_x;
    t_cursor_y = old_y;
    t_fg_color = old_fg;
}

bool init_terminal()
{
    t_fb = get_fb();
    t_fb_pitch = get_fb_pitch();
    t_fb_width = get_fb_width();
    t_fb_height = get_fb_height();

    t_cell_bg = kmalloc(
        sizeof(uint32_t) *
        TERMINAL_WIDTH *
        TERMINAL_HEIGHT
    );

    if (!t_cell_bg)
        return false;

    t_clear();
    return true;
}
