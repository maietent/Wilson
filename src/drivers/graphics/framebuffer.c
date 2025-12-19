#include "framebuffer.h"

uint32_t *fb_ptr = 0;
size_t width = 0;
size_t height = 0;
size_t pitch = 0;

uint32_t *get_fb(void)
{
    return fb_ptr;
}

size_t get_fb_width(void)
{
    return width;
}

size_t get_fb_height(void)
{
    return height;
}

size_t get_fb_pitch(void)
{
    return pitch;
}

bool init_framebuffer(void)
{
    struct limine_framebuffer *framebuffer = get_framebuffer();
    if(!framebuffer)
        return false;

    fb_ptr = framebuffer->address;
    width = framebuffer->width;
    height = framebuffer->height;
    pitch = framebuffer->pitch / 4;

    if(!fb_ptr || !width || !height || !pitch)
        return false;

    return true;
}
