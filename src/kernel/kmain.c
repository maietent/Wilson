#include "cpu_utils.h"
#include "framebuffer.h"
#include "limine.h"
#include "gdt.h"
#include "idt.h"
#include "umain.h"

void kmain(void)
{
    limine_init();
    gdt_init();
    idt_init();

    if (!init_framebuffer())
        CU_halt();

    umain();

    CU_halt();
}
