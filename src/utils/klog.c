#include "klog.h"
#include "printf.h"

static char klog_buffer[KLOG_BUFFER_SIZE];
static size_t klog_index = 0;
static void (*terminal_printf)(const char* fmt, ...) = NULL;

static void klog_append(const char* str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        klog_buffer[klog_index] = str[i];
        klog_index = (klog_index + 1) % KLOG_BUFFER_SIZE;
    }
}

void klog_set_terminal(void (*t_printf)(const char* fmt, ...)) {
    terminal_printf = t_printf;

    if (terminal_printf) {
        if (klog_index < KLOG_BUFFER_SIZE) {
            terminal_printf("%.*s", (int)klog_index, klog_buffer);
        } else {
            terminal_printf("%.*s", (int)(KLOG_BUFFER_SIZE - klog_index), &klog_buffer[klog_index]);
            terminal_printf("%.*s", (int)klog_index, klog_buffer);
        }
    }
}

void klogf(const char* fmt, ...) {
    char temp[512];
    char prefixed[512];

    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(temp, sizeof(temp), fmt, args);
    va_end(args);

    if (len > 0) {
        if ((size_t)len > sizeof(temp)) len = sizeof(temp);

        int prefixed_len = snprintf(prefixed, sizeof(prefixed), "[KLog] %.*s", len, temp);

        if ((size_t)prefixed_len > sizeof(prefixed)) prefixed_len = sizeof(prefixed);

        klog_append(prefixed, prefixed_len);

        if (terminal_printf) {
            terminal_printf("%.*s", prefixed_len, prefixed);
        }
    }
}

const char* klog_get_buffer(void) {
    return klog_buffer;
}

size_t klog_get_size(void) {
    return klog_index < KLOG_BUFFER_SIZE ? klog_index : KLOG_BUFFER_SIZE;
}
