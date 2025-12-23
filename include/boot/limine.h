#pragma once

#include "std.h"
#include <limine_lib.h>

void limine_init(void);
struct limine_framebuffer *get_framebuffer(void);
struct limine_memmap_response *get_memmap(void);
uint64_t get_hhdm(void);
