#include "cpu_utils.h"
#include "framebuffer.h"
#include "limine.h"
#include "moneycat.h"
#include "font.h"
#include "printf.h"
#include "gdt.h"
#include "idt.h"

void kmain(void)
{
    limine_init();
    gdt_init();
    idt_init();

    if(!init_framebuffer())
        CU_halt();

    uint32_t *fb = get_fb();
    size_t fb_width = get_fb_width();
    size_t fb_height = get_fb_height();
    size_t pitch = get_fb_pitch();

#ifdef DEBUG_BUILD
    bool is_debug = true;
#else
    bool is_debug = false;
#endif

    char buf[256];
    snprintf(buf, sizeof(buf), "fb_width: %d, fb_height: %d, is_debug: %d", fb_width, fb_height, is_debug);
    draw_string(fb, pitch, 1, 1, buf, 0xFFFFFF);

    CU_halt();
}
