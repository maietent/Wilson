#include "shell.h"
#include "terminal.h"
#include "klog.h"
#include "cursor.h"
#include "framebuffer.h"
#include "cpu_utils.h"
#include "string.h"

static uint32_t cursor_backing[CURSOR_WIDTH * CURSOR_HEIGHT];
static bool cursor_visible = false;
static int last_x = 0;
static int last_y = 0;

char cmd_buf[512];
size_t cmd_len = 0;

static void restore_cursor(uint32_t *fb, size_t pitch)
{
    if (!cursor_visible)
        return;

    for (int cy = 0; cy < CURSOR_HEIGHT; cy++)
    {
        for (int cx = 0; cx < CURSOR_WIDTH; cx++)
        {
            fb[(last_y + cy) * pitch + (last_x + cx)] =
                cursor_backing[cy * CURSOR_WIDTH + cx];
        }
    }

    cursor_visible = false;
}

static void draw_cursor(uint32_t *fb, size_t pitch, int x, int y)
{
    for (int cy = 0; cy < CURSOR_HEIGHT; cy++)
    {
        for (int cx = 0; cx < CURSOR_WIDTH; cx++)
        {
            cursor_backing[cy * CURSOR_WIDTH + cx] =
                fb[(y + cy) * pitch + (x + cx)];
        }
    }

    for (int cy = 0; cy < CURSOR_HEIGHT; cy++)
    {
        for (int cx = 0; cx < CURSOR_WIDTH; cx++)
        {
            uint32_t px = cursor_data[cy * CURSOR_WIDTH + cx];

            if ((px & 0x00FFFFFF) == 0)
                continue;

            fb[(y + cy) * pitch + (x + cx)] = px;
        }
    }

    last_x = x;
    last_y = y;
    cursor_visible = true;
}

void s_draw_prompt(void)
{
    t_printf("\n> ");
}

void s_command_handler()
{
    if (strcmp(cmd_buf, "help") == 0) {
        t_printf("\nCommands:\necho\nhelp\nklog\n");
    }
    else if (strncmp(cmd_buf, "echo", 4) == 0) {
        char tmp[512] = {0};
        if (strlen(cmd_buf) > 5) {
            strncpy(tmp, cmd_buf + 5, 512 - 1);
        }

        t_printf("\n%s\n", tmp);
    }
    else if (strcmp(cmd_buf, "klog") == 0) {
        t_printf("\n");
        klog_flush();
    }
    else {
        t_printf("\nUnknown command: %s\n", cmd_buf);
    }
}

void s_handle_enter()
{
    if (!cmd_len)
    {
        s_draw_prompt();
        return;
    }

    s_command_handler();
    //t_printf("\nYour command: %s\n", cmd_buf);

    s_draw_prompt();
    cmd_len = 0;
    cmd_buf[0] = '\0';
}

void s_handle_backspace()
{
    if (cmd_len)
    {
        t_backspace();
        cmd_buf[cmd_len - 1] = '\0';
        cmd_len--;
    }
}

void s_handle_keyboard(char key_char, uint8_t scancode)
{
    (void)scancode; // the warning is pissing me off

    if (key_char == '\n')
    {
        s_handle_enter();
        return;
    }

    if (key_char == '\b')
    {
        s_handle_backspace();
        return;
    }

    if (cmd_len < sizeof(cmd_buf) - 1)
    {
        cmd_buf[cmd_len++] = key_char;
        cmd_buf[cmd_len] = '\0';
    }

    t_printf("%c", key_char);
}

void s_handle_mouse(int cursor_x, int cursor_y)
{
    uint32_t *fb = get_fb();
    size_t pitch = get_fb_pitch();

    restore_cursor(fb, pitch);
    draw_cursor(fb, pitch, cursor_x, cursor_y);
}

void shell_run(void)
{
    t_clear();
    t_draw_header(
        "Wilson",
        0xFFFFFF,
        0x0000DD);

    s_draw_prompt();

    CU_halt();
}
