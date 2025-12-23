#include "memmap.h"
#include "limine.h"

#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))

extern uintptr_t kernel_phys_end;

uintptr_t find_usable_mem(size_t min_size)
{
    uintptr_t min_addr = ((uintptr_t)&kernel_phys_end - 0xffffffff80000000);
    min_addr = ALIGN_UP(min_addr, 0x1000);

    struct limine_memmap_response *memmap = get_memmap();

    for (size_t i = 0; i < memmap->entry_count; i++)
    {
        struct limine_memmap_entry *e = memmap->entries[i];
        if (e->type != LIMINE_MEMMAP_USABLE)
            continue;

        uintptr_t start = (uintptr_t)e->base;
        uintptr_t length = (uintptr_t)e->length;

        if (start < min_addr)
        {
            size_t skip = min_addr - start;
            if (skip >= length) continue;
            start += skip;
            length -= skip;
        }

        if (length >= min_size)
            return start;
    }

    return 0;
}
