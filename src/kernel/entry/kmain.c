#include "cpu_utils.h"
#include "framebuffer.h"
#include "limine.h"
#include "gdt.h"
#include "idt.h"
#include "memmap.h"
#include "alloc.h"
#include "sse.h"
#include "klog.h"
#include "driver.h"
#include "umain.h"

void kmain(void)
{
    klogf("Entered kmain\n");

#ifdef DEBUG_BUILD
    klogf("Debug build\n");
#endif

    limine_init();
    klogf("Limine initialized\n");

    gdt_init();
    klogf("GDT initialized\n");

    idt_init();
    klogf("IDT initialized\n");

    //load_drivers();
    //klogf("Drivers initialized\n");

    enable_sse();
    klogf("SSE initialized\n");

    asm volatile ("sti");
    klogf("Interrupts enabled\n");

    uintptr_t heap_base = find_usable_mem(1024 * 1024);
    if (!heap_base)
        CU_halt();

    init_heap(heap_base + get_hhdm());
    klogf("Heap initialized\n");

    if (!init_framebuffer())
        CU_halt();

    klogf("Framebuffer initialized\n");

    umain();

    klogf("Exited umain\n");
    CU_halt();
}
