#include "umain.h"
#include "framebuffer.h"
#include "terminal.h"
#include "alloc.h"
#include "cpu_utils.h"
#include "klog.h"
#include "keyboard.h"
#include "mouse.h"

void kb_handler(char key_char, uint8_t scancode)
{
    (void)scancode;
    klogf("test %c\n", key_char);
}

void umain(void)
{
    klogf("Entered umain\n");

    klogf("Framebuffer resolution: %dx%d\n", get_fb_width(), get_fb_height());

    init_terminal();
    klogf("Terminal initialized\n");

    klog_set_terminal(t_printf, t_set_color);
    klogf("Terminal selected for klog\n");

    keyboard_init(kb_handler);

    CU_halt();
}
