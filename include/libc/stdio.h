#pragma once

#include <stdarg.h>
#include <stddef.h>

#include "printf.h"

int vsnprintf(char* buffer, size_t size, const char* format, va_list args);
void itoa(int value, char* buffer, int base);