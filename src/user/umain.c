#include "umain.h"
#include "framebuffer.h"
#include "terminal.h"

void umain(void)
{
    init_terminal();
    t_drawstring("test\ntest\n");
    t_printf("printf test, res: %dx%d\n", get_fb_width(), get_fb_height());
    t_scroll();
    t_set_color(0xFFFF00);
    t_printf("scrolled! and color!");
}
