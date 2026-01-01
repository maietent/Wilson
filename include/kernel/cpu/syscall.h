#pragma once

#include "std.h"

static inline void syscall_print(char c)
{
    asm volatile (
        "int $0x80"
        :
        : "a"(1), "b"(c)
        : "memory"
    );
}

void syscall_handler_c(uint64_t rax, uint64_t rbx, uint64_t rcx);
void syscall_init(void);
