#include "alloc.h"

#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))

static uintptr_t heap_ptr;

void init_heap(uintptr_t start)
{
    heap_ptr = start;
}

void *kmalloc(size_t size)
{
    heap_ptr = ALIGN_UP(heap_ptr, 8);
    void *p = (void *)heap_ptr;
    heap_ptr += size;
    return p;
}
