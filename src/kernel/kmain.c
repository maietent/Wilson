#include "cpu_utils.h"
#include "framebuffer.h"
#include "limine.h"
#include "gdt.h"
#include "idt.h"
#include "memmap.h"
#include "alloc.h"
#include "sse.h"
#include "umain.h"

void kmain(void)
{
    limine_init();
    gdt_init();
    idt_init();

    enable_sse();

    uintptr_t heap_base = find_usable_mem(1024 * 1024);
    if (!heap_base)
        CU_halt();

    init_heap(heap_base + get_hhdm());

    if (!init_framebuffer())
        CU_halt();

    umain();

    CU_halt();
}
