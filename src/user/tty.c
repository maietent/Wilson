#include "tty.h"
#include "terminal.h"
#include "klog.h"
#include "string.h"
#include "syscall.h"
#include "cpu_utils.h"
#include "libc.h"
#include "keyboard.h"
#include "ata.h"
#include "stdio.h"

char t_cmd_buf[TTY_CMD_BUF_SIZE];
size_t t_cmd_len = 0;

char t_history[TTY_HISTORY_SIZE][TTY_CMD_BUF_SIZE];
int t_history_count = 0;
int t_history_index = -1;

char* t_command_args = NULL;

t_command_t t_commands[] = {
    { "help", "List all available commands", t_cmd_help },
    { "echo", "Print text to the terminal", t_cmd_echo },
    { "clear", "Clear the terminal", t_cmd_clear },
    { "reboot", "Reboot the system", t_cmd_reboot },
    { "sleep", "Sleep for 5 seconds", t_cmd_sleep },
    { "klog", "Print kernel log", t_cmd_klog },
    { "syscallc", "Test syscall", t_cmd_syscallc },
    { "ataread", "Read ATA sectors and dump bytes. Usage: ataread <lba> [offset]", t_cmd_ata_read },
};

size_t t_command_count = sizeof(t_commands) / sizeof(t_commands[0]);

void t_cmd_help(void)
{
    t_printf("Available commands\n");
    for (size_t i = 0; i < t_command_count; i++)
        t_printf("%s: %s\n", t_commands[i].name, t_commands[i].description);
}

void t_cmd_echo(void)
{
    if (t_command_args && *t_command_args)
        t_printf(t_command_args);
    t_printf("\n");
}

void t_cmd_clear(void)
{
    t_clear();
}

void t_cmd_reboot(void)
{
    t_printf("Rebooting system\n");
    asm volatile("cli\nmovb $0xFE, %al\noutb %al, $0x64\nhlt\n");
}

void t_cmd_sleep(void)
{
    for (int i = 0; i < 5; i++)
    {
        klogf("Tick %d\n", i);
        sleep_ms(1000);
    }
}

void t_cmd_klog(void)
{
    klog_flush();
    t_printf("\n");
}

void t_cmd_syscallc(void)
{
    syscall_print('a');
    t_printf("\n");
}

void t_cmd_ata_read(void)
{
    if (!t_command_args) {
        t_printf("Usage: ataread <lba> [offset]\n");
        return;
    }

    // parse arguments
    char* arg1 = strtok(t_command_args, " ");
    char* arg2 = strtok(NULL, "");

    if (!arg1) {
        t_printf("Missing LBA\n");
        return;
    }

    uint32_t lba = kstrtoul(arg1);
    uint32_t offset = arg2 ? kstrtoul(arg2) : 0;

    if (offset >= 512) {
        t_printf("Offset must be 0-511\n");
        return;
    }

    uint8_t buffer[512];
    if (!ata_read_sector(lba, buffer)) {
        t_printf("ATA read failed at LBA %u\n", lba);
        return;
    }

    t_printf("Sector %u bytes (offset %u):\n", lba, offset);

    for (uint32_t i = offset; i < 512; i++) {
        t_printf("%02X ", buffer[i]);
        if ((i - offset + 1) % 16 == 0)
            t_printf("\n");
    }
    t_printf("\n");
}

void t_clear_input_field(size_t input_len)
{
    for (size_t i = 0; i < input_len; i++)
    {
        t_putchar('\b');
        t_putchar(' ');
        t_putchar('\b');
    }
}

int t_input_only_spaces(const char* input, size_t len)
{
    for (size_t i = 0; i < len; i++)
        if (input[i] != ' ')
            return 0;
    return 1;
}

void t_draw_prompt(void)
{
    t_printf("\n$ ");
}

void t_history_up(void)
{
    if (t_history_count > 0)
    {
        if (t_history_index == -1)
            t_history_index = 0;
        else if (t_history_index < t_history_count - 1)
            t_history_index++;

        t_clear_input_field(t_cmd_len);
        strcpy(t_cmd_buf, t_history[t_history_count - 1 - t_history_index]);
        t_cmd_len = strlen(t_cmd_buf);
        t_printf(t_cmd_buf);
    }
}

void t_history_down(void)
{
    if (t_history_index > 0)
    {
        t_history_index--;
        t_clear_input_field(t_cmd_len);
        strcpy(t_cmd_buf, t_history[t_history_count - 1 - t_history_index]);
        t_cmd_len = strlen(t_cmd_buf);
        t_printf(t_cmd_buf);
    }
    else if (t_history_index == 0)
    {
        t_history_index = -1;
        t_clear_input_field(t_cmd_len);
        t_cmd_len = 0;
        t_cmd_buf[0] = '\0';
    }
}

void t_command_handler(void)
{
    if (!t_cmd_len || t_input_only_spaces(t_cmd_buf, t_cmd_len))
    {
        t_printf("\n");
        return;
    }

    t_cmd_buf[t_cmd_len] = '\0';

    if (t_history_count == 0 || strcmp(t_history[t_history_count - 1], t_cmd_buf) != 0)
    {
        if (t_history_count < TTY_HISTORY_SIZE)
        {
            strcpy(t_history[t_history_count], t_cmd_buf);
            t_history_count++;
        }
        else
        {
            for (int i = 1; i < TTY_HISTORY_SIZE; i++)
                strcpy(t_history[i - 1], t_history[i]);
            strcpy(t_history[TTY_HISTORY_SIZE - 1], t_cmd_buf);
        }
    }

    t_printf("\n");

    char* cmd_name = strtok(t_cmd_buf, " ");
    t_command_args = strtok(NULL, "");

    int found = 0;
    for (size_t i = 0; i < t_command_count; i++)
    {
        if (strcmp(cmd_name, t_commands[i].name) == 0)
        {
            t_commands[i].function();
            found = 1;
            break;
        }
    }

    if (!found)
        t_printf("Unknown command: %s\n", cmd_name);
}

void t_handle_keyboard(char key_char, uint8_t scancode)
{
    if (key_char == '\n')
    {
        t_command_handler();
        t_cmd_len = 0;
        t_cmd_buf[0] = '\0';
        t_history_index = -1;
        t_draw_prompt();
        return;
    }

    if (key_char == '\b')
    {
        if (t_cmd_len > 0)
        {
            t_cmd_len--;
            t_handle_backspace();
        }
        return;
    }

    if (scancode == 0x48) { t_history_up(); return; }
    if (scancode == 0x50) { t_history_down(); return; }

    if (t_cmd_len < sizeof(t_cmd_buf) - 1)
    {
        t_cmd_buf[t_cmd_len++] = key_char;
        t_cmd_buf[t_cmd_len] = '\0';
        t_printf("%c", key_char);
    }
}

void t_handle_backspace(void)
{
    t_clear_input_field(1);
}

void t_handle_enter(void)
{
    t_command_handler();
    t_cmd_len = 0;
    t_cmd_buf[0] = '\0';
    t_history_index = -1;
    t_draw_prompt();
}
