#ifndef FB_H
#define FB_H
#include <stdint.h>
typedef struct {
    uint32_t *address;
    uint32_t width, height, pitch, bpp;
    int available;
} framebuffer_t;
extern framebuffer_t fb;
void fb_init(uint32_t magic, uint32_t mbi);
void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void fb_clear(uint32_t color);
void fb_draw_char(uint32_t x, uint32_t y, char c, uint32_t fg, uint32_t bg);
void fb_draw_text(uint32_t x, uint32_t y, const char *t, uint32_t fg, uint32_t bg);
#endif