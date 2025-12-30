#pragma once

#include "std.h"

#define KLOG_BUFFER_SIZE 4096

#define klog klogf

void klog_set_terminal(void (*t_printf)(const char* fmt, ...), void (*t_set_color)(uint32_t color));

void klog_flush(void);
void klogf(const char* fmt, ...);

const char* klog_get_buffer(void);
size_t klog_get_size(void);
