#ifndef VGA_H
#define VGA_H

#include <stdint.h>
#include "mmio.h"

/* Color constants */
#define VGA_COLOR_BLUE   0x03u
#define VGA_COLOR_RED    0xE0u
#define VGA_COLOR_GREEN  0xFAu

// Memory-mapped VGA constants
#define VGA_ADDR      MMIO32(0x11000120u)
#define VGA_COLOR_REG MMIO32(0x11000140u)
#define VGA_RD_COLOR  MMIO32(0x11000180u)  /* optional */

/* function headers */
void vga_draw_pixel(uint32_t x, uint32_t y, uint32_t color);
void vga_fill(uint32_t color);

#endif