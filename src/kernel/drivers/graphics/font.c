#include "font.h"
#include "fontarrays.h"
#include "framebuffer.h"

void draw_char(uint32_t *fb, size_t pitch, size_t x, size_t y, char c, uint32_t color)
{
    if ((unsigned char) c > 127) return;

    size_t font_height = 14;
    const uint8_t *glyph = &AVGA2_8x14[(unsigned char)c * font_height];

    for (size_t row = 0; row < font_height; row++)
    {
        for (size_t col = 0; col < 8; col++)
        {
            if (c == ' ' || !(glyph[row] & (1 << (7- col))))
            {
                fb_draw_pixel(fb, pitch, x + col, y + row, 0x000000);
            }
            else
            {
                fb_draw_pixel(fb, pitch, x + col, y + row, color);
            }
        }
    }

}

void draw_string(uint32_t *fb, size_t pitch, size_t x, size_t y, const char* str, uint32_t color)
{
    while(*str)
    {
        draw_char(fb, pitch, x, y, *str, color);
        x += 8;
        str++;
    }
}
