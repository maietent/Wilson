#pragma once

#include "std.h"

#include "printf.h"

int vsnprintf(char* buffer, size_t size, const char* format, va_list args);
void itoa(int value, char* buffer, int base);
uint32_t kstrtoul(const char* str);
