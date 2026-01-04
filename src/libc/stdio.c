#include "stdio.h"
#include "string.h"

void itoa(int value, char* buffer, int base) {
    char* digits = "0123456789ABCDEF";
    char temp[32];
    int i = 0, j = 0, is_negative = 0;

    if (value < 0 && base == 10) {
        is_negative = 1;
        value = -value;
    }

    do {
        temp[i++] = digits[value % base];
        value /= base;
    } while (value > 0);

    if (is_negative) {
        temp[i++] = '-';
    }

    while (i--) {
        buffer[j++] = temp[i];
    }
    buffer[j] = '\0';
}

uint32_t kstrtoul(const char* str)
{
    uint32_t result = 0;
    if (!str) return 0;

    while (*str) {
        char c = *str++;
        if (c >= '0' && c <= '9') {
            result = result * 10 + (c - '0');
        } else {
            break;
        }
    }
    return result;
}
