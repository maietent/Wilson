#include "t_shell.h"
#include "terminal.h"
#include "klog.h"
#include "string.h"
#include "syscall.h"
#include "cpu_utils.h"

char t_cmd_buf[512];
size_t t_cmd_len = 0;

void t_draw_prompt(void)
{
    t_printf("\n> ");
}

void t_command_handler(void)
{
    if (strcmp(t_cmd_buf, "help") == 0)
    {
        t_printf("\nCommands:\necho\nhelp\nklog\n");
    }
    else if (strncmp(t_cmd_buf, "echo", 4) == 0)
    {
        char tmp[512] = {0};
        if (strlen(t_cmd_buf) > 5)
            strncpy(tmp, t_cmd_buf + 5, 512 - 1);
        t_printf("\n%s\n", tmp);
    }
    else if (strcmp(t_cmd_buf, "klog") == 0)
    {
        t_printf("\n");
        klog_flush();
    }
    else if (strcmp(t_cmd_buf, "sleep") == 0)
    {
        t_printf("\n");
        for (int i = 0; i < 5; i++)
        {
            klogf("Tick %d\n", i);
            sleep_ms(1000);
        }
    }
    else if (strcmp(t_cmd_buf, "syscallc") == 0)
    {
        t_printf("\n");
        syscall_print('a');
        t_printf("\n");
    }
    else
    {
        t_printf("\nUnknown command: %s\n", t_cmd_buf);
    }
}

void t_handle_enter(void)
{
    if (!t_cmd_len)
    {
        t_draw_prompt();
        return;
    }

    t_command_handler();
    t_draw_prompt();
    t_cmd_len = 0;
    t_cmd_buf[0] = '\0';
}

void t_handle_backspace(void)
{
    if (t_cmd_len)
    {
        t_backspace();
        t_cmd_buf[t_cmd_len - 1] = '\0';
        t_cmd_len--;
    }
}

void t_handle_keyboard(char key_char, uint8_t scancode)
{
    (void)scancode; //warning

    if (key_char == '\n')
    {
        t_handle_enter();
        return;
    }

    if (key_char == '\b')
    {
        t_handle_backspace();
        return;
    }

    if (t_cmd_len < sizeof(t_cmd_buf) - 1)
    {
        t_cmd_buf[t_cmd_len++] = key_char;
        t_cmd_buf[t_cmd_len] = '\0';
    }

    t_printf("%c", key_char);
}
