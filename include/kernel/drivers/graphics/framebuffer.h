#pragma once

#include "std.h"
#include "limine.h"

extern uint32_t *fb_ptr;
extern size_t width;
extern size_t height;
extern size_t pitch;

uint32_t *get_fb(void);
size_t get_fb_width(void);
size_t get_fb_height(void);
size_t get_fb_pitch(void);

bool init_framebuffer(void);
void fb_draw_pixel(uint32_t *fb, size_t pitch, size_t x, size_t y, uint32_t color);
void fb_clear();
