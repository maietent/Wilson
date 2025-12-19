#include "cpu_utils.h"
#include "framebuffer.h"
#include "limine.h"

void kmain(void)
{
    limine_init();

    if(!init_framebuffer())
        CU_halt();

    uint32_t *fb = get_fb();
    fb[0] = 0xFFFFFFFF;

    CU_halt();
}
