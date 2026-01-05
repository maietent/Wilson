#include "umain.h"
#include "framebuffer.h"
#include "terminal.h"
#include "alloc.h"
#include "cpu_utils.h"
#include "klog.h"
#include "shell.h"

void umain(void)
{
    klogf("Entered umain\n");

    klogf("Framebuffer resolution: %dx%d\n", get_fb_width(), get_fb_height());

    init_terminal();
    klogf("Terminal initialized\n");

    klog_set_terminal(t_printf, t_set_color);
    klogf("Terminal selected for klog\n");

    klogf("Running shell\n");
    shell_run();

    CU_halt();
}
