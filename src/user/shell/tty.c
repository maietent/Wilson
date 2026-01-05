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
#include "pit.h"
#include "fat32.h"

char t_cmd_buf[TTY_CMD_BUF_SIZE];
size_t t_cmd_len = 0;

char t_history[TTY_HISTORY_SIZE][TTY_CMD_BUF_SIZE];
int t_history_count = 0;
int t_history_index = -1;

char* t_command_args = NULL;

int cursor_tick = 0;
bool show_cursor = true;

t_command_t t_commands[] = {
    { "help", "List all available commands", t_cmd_help },
    { "echo", "Print text to the terminal", t_cmd_echo },
    { "clear", "Clear the terminal", t_cmd_clear },
    { "reboot", "Reboot the system", t_cmd_reboot },
    { "getticks", "Get the current PIT ticks", t_cmd_getticks },
    { "sleep", "Sleep for 5 seconds", t_cmd_sleep },
    { "klog", "Print kernel log", t_cmd_klog },
    { "syscallc", "Test syscall", t_cmd_syscallc },
    { "ataread", "Read ATA sectors and dump bytes. Usage: ataread <lba> [offset]", t_cmd_ata_read },
    { "ls", "List files in current directory", t_cmd_ls },
    { "cd", "Change directory. Usage: cd <dirname>", t_cmd_cd },
    { "cat", "Display file contents. Usage: cat <filename>", t_cmd_cat },
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

void t_cmd_getticks(void)
{
    t_printf("\n");
    t_printf("%d", pit_get_ticks());
    t_printf("\n");
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

static void ls_callback(const char *name, uint32_t size, uint8_t attr)
{
    if (attr & FAT32_ATTR_DIRECTORY)
        t_printf("[DIR]  %s\n", name);
    else
        t_printf("       %s (%u bytes)\n", name, size);
}

void t_cmd_ls(void)
{
    if (!fat32_list_dir(NULL, ls_callback)) {
        t_printf("Failed to list directory\n");
    }
}

void t_cmd_cd(void)
{
    if (!t_command_args || !*t_command_args) {
        t_printf("Usage: cd <directory>\n");
        return;
    }

    if (!fat32_change_dir(t_command_args)) {
        t_printf("Directory not found: %s\n", t_command_args);
    }
}

void t_cmd_cat(void)
{
    if (!t_command_args || !*t_command_args) {
        t_printf("Usage: cat <filename>\n");
        return;
    }

    fat32_file_t file;
    if (!fat32_open(t_command_args, &file)) {
        t_printf("Failed to open file: %s\n", t_command_args);
        return;
    }

    if (file.attributes & FAT32_ATTR_DIRECTORY) {
        t_printf("%s is a directory\n", t_command_args);
        fat32_close(&file);
        return;
    }

    if (file.file_size == 0) {
        t_printf("(empty file)\n");
        fat32_close(&file);
        return;
    }

    uint8_t buffer[513];
    uint32_t bytes_read = 0;
    int result;

    while ((result = fat32_read(&file, buffer, 512)) > 0) {
        buffer[result] = '\0';
        t_printf("%s", (char*)buffer);
        bytes_read += result;
    }

    if (result < 0) {
        t_printf("\nError reading file\n");
    } else if (bytes_read > 0) {
        t_printf("\n");
    }

    fat32_close(&file);
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
        t_printf("%s", t_cmd_buf);
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
        t_printf("%s", t_cmd_buf);
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
    if (scancode == 0x48) { t_history_up(); return; }
    if (scancode == 0x50) { t_history_down(); return; }

    if (key_char == 0)
        return;

    if (key_char == '\n')
    {
        t_handle_enter();
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
    if (show_cursor)
        t_putchar_at_coord(' ', get_cursor_x(), get_cursor_y());

    t_command_handler();
    t_cmd_len = 0;
    t_cmd_buf[0] = '\0';
    t_history_index = -1;
    t_draw_prompt();
}

void t_toggle_cursor()
{
    show_cursor = !show_cursor;

    if (show_cursor)
        t_putchar_at_coord('_', get_cursor_x(), get_cursor_y());
    else
        t_putchar_at_coord(' ', get_cursor_x(), get_cursor_y());
}

static uint32_t old_pit_ticks = 0;

void t_cursor_tick()
{
    uint32_t pit_ticks = pit_get_ticks();

    if (pit_ticks - old_pit_ticks >= 250)
    {
        t_toggle_cursor();
        old_pit_ticks = pit_ticks;
        return;
    }
}

void t_tick()
{
    t_cursor_tick();
}
