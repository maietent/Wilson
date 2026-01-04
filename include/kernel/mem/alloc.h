#pragma once

#include "std.h"

#define HEAP_SIZE (32 * 1024 * 1024)

void init_heap(uintptr_t start, size_t size);
void *kmalloc(size_t size);
void kfree(void *ptr);
void *krealloc(void *ptr, size_t size);
