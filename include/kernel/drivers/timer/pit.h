#pragma once

#include "std.h"

void pit_init(uint32_t frequency);
void pit_handler_c(void);
uint64_t pit_get_ticks(void);
void pit_sleep_ms(uint64_t ms);
