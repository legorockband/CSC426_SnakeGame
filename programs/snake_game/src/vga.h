#ifndef VGA_H
#define VGA_H

#include <stdint.h>

/* Color constants */
#define VGA_COLOR_BLUE   0x03u
#define VGA_COLOR_RED    0xE0u
#define VGA_COLOR_GREEN  0xFAu

/* function headers */
void vga_draw_pixel(uint32_t x, uint32_t y, uint32_t color);
void vga_fill(uint32_t color);

#endif