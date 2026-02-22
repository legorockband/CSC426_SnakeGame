// compile-time (no memory) constant definitions:
// examples:
//   #define MY_CONST 7

// section for global data memory allocation
// examples:
//   // reserve global memory for one variable, and define the initial value
//   int my_global_var = 55;
//   // reserve global memory for an array of 4 words, and define the initial values
//   int my_global_array[] = {77, 0b10101010111100001100110011111111, 0xFFFFFFFF, 99};
//   // reserve global memory for an array of 6 words, without defining the initial values (initialized to 0's on some systems)
//   int my_global_array_uninit[6];


// section for code

#include <stdint.h>

#define VGA_ADDR      (*(volatile uint32_t*)0x11000120u)
#define VGA_COLOR     (*(volatile uint32_t*)0x11000140u)
#define VGA_RD_COLOR  (*(volatile uint32_t*)0x11000180u) // optional

// pick any 8-bit color value that your VGA hardware expects
// examples from the asm:
// 0x03 = blue-ish, 0xE0 = red-ish, 0xFA = light green-ish
#define BACK_COLOR 0x03u

static inline void vga_draw_pixel(uint32_t x, uint32_t y, uint32_t color) {
    // match asm masks: x[6:0], y[5:0]
    x &= 0x7Fu;
    y &= 0x3Fu;

    uint32_t addr = (y << 7) | x;   // 13-bit address
    VGA_ADDR  = addr;               // latch address
    VGA_COLOR = color;              // write pixel color
}

int main(void) {
    // Fill a 128x64 logical framebuffer (matches x=7 bits, y=6 bits)
    // If your actual visible area is smaller (e.g., 80x60), reduce bounds.
    for (uint32_t y = 0; y < 64; y++) {
        for (uint32_t x = 0; x < 128; x++) {
            vga_draw_pixel(x, y, BACK_COLOR);
        }
    }

    // stay alive forever
    while (1) { }
    return 0;
}