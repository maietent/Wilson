#pragma once

#include "std.h"

void pit_init(uint32_t frequency);
uint64_t pit_get_ticks(void);
void pit_wait(uint32_t ticks);
