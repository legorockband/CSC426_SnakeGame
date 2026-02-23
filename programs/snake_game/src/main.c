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
#include "vga.h"
#include "inputs.h"
#include "outputs.h"

#define COUNT_DOWN_START 3
#define SW0_MASK 0x0001u             // To add other switchs just add 1 to the hex value 

#define SNAKE_POSX 20
#define SNAKE_POSY 30
#define FRUIT_POSX 60
#define FRUIT_POSY 30

typedef enum { WAIT_START, PLAYING } GameState;

static void delay_cycles(volatile uint32_t cycles)
{
    while (cycles--) {
        // empty loop; 'volatile' prevents optimization
    }
}

// Rough ~1 second delay
static void delay_ms(uint32_t ms)
{
    while (ms--) {
        delay_cycles(10000);
    }
}

// Count down from a number 
static void count_down(uint8_t count_down){
    while (count_down != 0){
        set_sseg(count_down);
        delay_ms(1000);
        count_down--;
    }  
    set_sseg(0);  
}

// Create the snake, fruit, and background
static void init_game(){
    /* Fill background */
    vga_fill(VGA_COLOR_GREEN);

    // Snake
    vga_draw_pixel(SNAKE_POSX, SNAKE_POSY, VGA_COLOR_BLUE);

    // Fruit
    vga_draw_pixel(FRUIT_POSX, FRUIT_POSY, VGA_COLOR_RED);
}

int main(void) {
    init_game();

    GameState state = WAIT_START;
    uint16_t prev_sw = 0;
    uint8_t score_count = 0;

    while (1) {
        uint16_t sw = read_switches();
        uint8_t sw0 = (sw & SW0_MASK) ? 1 : 0;
        uint8_t prev_sw0 = (prev_sw & SW0_MASK) ? 1 : 0;
        prev_sw = sw;

        if (state == WAIT_START) {
            set_sseg(score_count);
            if (sw0 && !prev_sw0) {
                count_down(COUNT_DOWN_START);
                state = PLAYING;
            }
        } else { // PLAYING
            // next: movement, collision, scoring
            set_sseg(score_count);
        }
    }

    return 0;
}