#include "vga.h"

// Memory-mapped VGA constants
#define VGA_ADDR      (*(volatile uint32_t*)0x11000120u)
#define VGA_COLOR_REG (*(volatile uint32_t*)0x11000140u)
#define VGA_RD_COLOR  (*(volatile uint32_t*)0x11000180u)  /* optional */

/* address helper function */
static inline uint32_t vga_compute_addr(uint32_t x, uint32_t y) {
    x &= 0x7Fu;   /* x[6:0] */
    y &= 0x3Fu;   /* y[5:0] */
    return (y << 7) | x;
}


void vga_draw_pixel(uint32_t x, uint32_t y, uint32_t color) {
    /*
    Draw pixel on screen
    */
    uint32_t addr = vga_compute_addr(x, y);
    VGA_ADDR      = addr;
    VGA_COLOR_REG = color;
}

void vga_fill(uint32_t color) {

    /*
    fill screen with color 
    */

    for (uint32_t y = 0; y < 64; y++) {
        for (uint32_t x = 0; x < 128; x++) {
            vga_draw_pixel(x, y, color);
        }
    }
}