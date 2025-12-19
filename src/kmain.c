#include "cpu_utils.h"
#include "framebuffer.h"
#include "limine.h"
#include "moneycat.h"
#include "font.h"
#include "printf.h"

void kmain(void)
{
    limine_init();

    if(!init_framebuffer())
        CU_halt();

    uint32_t *fb = get_fb();
    size_t fb_width = get_fb_width();
    size_t fb_height = get_fb_height();
    size_t pitch = get_fb_pitch();

    //fb[0] = 0xFFFFFFFF;

    char buf[256];
    sprintf(buf, "fb_width: %i, fb_height: %i", fb_width, fb_height);
    draw_string(fb, pitch, 1, 1, buf, 0xFFFFFF);

    CU_halt();
}
