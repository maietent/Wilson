#include "klog.h"
#include "printf.h"
#include "string.h"
#include "ports.h"

static char klog_buffer[KLOG_BUFFER_SIZE];
static size_t klog_index = 0;
static void (*terminal_printf)(const char* fmt, ...) = NULL;
static void (*terminal_set_color)(uint32_t color) = NULL;

#define SERIAL_PORT 0x3F8

static void serial_init(void)
{
    outb(SERIAL_PORT + 1, 0x00);
    outb(SERIAL_PORT + 3, 0x80);
    outb(SERIAL_PORT + 0, 0x03);
    outb(SERIAL_PORT + 1, 0x00);
    outb(SERIAL_PORT + 3, 0x03);
    outb(SERIAL_PORT + 2, 0xC7);
    outb(SERIAL_PORT + 4, 0x0B);
}

static int serial_is_transmit_empty(void)
{
    return inb(SERIAL_PORT + 5) & 0x20;
}

static void serial_putchar(char c)
{
    while (!serial_is_transmit_empty());
    outb(SERIAL_PORT, c);
}

static void serial_write(const char* str, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        serial_putchar(str[i]);
    }
}

static void klog_append(const char* str, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        klog_buffer[klog_index] = str[i];
        klog_index = (klog_index + 1) % KLOG_BUFFER_SIZE;
    }

    serial_write(str, len);
}

void klog_set_terminal(void (*t_printf)(const char* fmt, ...),
    void (*t_set_color)(uint32_t color))
{
    static int serial_initialized = 0;
    if (!serial_initialized)
    {
        serial_init();
        serial_initialized = 1;
    }

    terminal_printf = t_printf;
    terminal_set_color = t_set_color;
}

void klog_flush(void)
{
    if (!terminal_printf || !terminal_set_color)
        return;

    size_t size = klog_get_size();
    const char* buf = klog_buffer;

    size_t i = 0;
    while (i < size)
    {
        size_t line_start = i;

        while (i < size && buf[i] != '\n')
            i++;

        size_t line_len = i - line_start;
        const char* line = &buf[line_start];
        const char prefix[] = "[KLog] ";
        size_t prefix_len = sizeof(prefix) - 1;

        if (line_len >= prefix_len &&
            strncmp(line, prefix, prefix_len) == 0)
        {
            terminal_set_color(0x00FFFF);
            terminal_printf("%.*s", (int)prefix_len, line);
            terminal_set_color(0xCCCCCC);
            terminal_printf("%.*s",
                (int)(line_len - prefix_len),
                line + prefix_len
            );
        }
        else
        {
            terminal_set_color(0xCCCCCC);
            terminal_printf("%.*s", (int)line_len, line);
        }
        if (i < size && buf[i] == '\n')
        {
            terminal_printf("\n");
            i++;
        }
    }

    terminal_set_color(0xFFFFFF);
}

void klogf(const char* fmt, ...)
{
    char temp[512];

    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(temp, sizeof(temp), fmt, args);
    va_end(args);

    if (len <= 0)
        return;

    if ((size_t)len > sizeof(temp))
        len = sizeof(temp);

    char prefixed[512];

    int prefixed_len = snprintf(prefixed, sizeof(prefixed), "[KLog] %.*s", len, temp);
    if ((size_t)prefixed_len > sizeof(prefixed))
        prefixed_len = sizeof(prefixed);

    klog_append(prefixed, prefixed_len);

    if (terminal_printf && terminal_set_color)
    {
        terminal_set_color(0x00FFFF);
        terminal_printf("[KLog] ");
        terminal_set_color(0xCCCCCC);
        terminal_printf("%.*s", len, temp);
        terminal_set_color(0xFFFFFF);
    }
}

const char* klog_get_buffer(void)
{
    return klog_buffer;
}

size_t klog_get_size(void)
{
    return klog_index < KLOG_BUFFER_SIZE ? klog_index : KLOG_BUFFER_SIZE;
}
