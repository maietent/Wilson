#include "umain.h"
#include "framebuffer.h"
#include "terminal.h"
#include "alloc.h"
#include "cpu_utils.h"
#include "klog.h"

void umain(void)
{
    klogf("Entered umain\n");

    init_terminal();
    klogf("Terminal initialized\n");

    klog_set_terminal(t_printf);
    klogf("Terminal selected for klog\n");

    t_drawstring("test\ntest\n");
    t_printf("printf test, res: %dx%d\n", get_fb_width(), get_fb_height());
    t_scroll();
    t_set_color(0xFF00FF);
    t_printf("scrolled! and color!\n");
    t_set_color(0xFFFFFF);
    t_printf("we shall test kmalloc...\n");
    int *buf = (int*)kmalloc(sizeof(int) * 4);
    if (!buf)
    {
        t_printf("kmalloc failed :(\n");
        CU_halt();
    }

    t_printf("kmalloc succeeded! addr = 0x%p\n", buf);

    t_printf("writing....\n");
    for (int i = 0; i < 4; i++) {
        buf[i] = i * 10;
    }

    klogf("kernel log\n");

    t_printf("readback: ");
    for (int i = 0; i < 4; i++) {
        t_printf("%d ", buf[i]);
    }
    t_printf("\n");

    CU_halt();
}
