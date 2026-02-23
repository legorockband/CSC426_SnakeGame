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

#define SNAKE_POSX 20
#define SNAKE_POSY 30
#define SNAKE_MAX_SIZE 256      // Subject to changed due to screen size

#define FRUIT_POSX 60
#define FRUIT_POSY 30

typedef enum { WAIT_START, PLAYING } GameState;

typedef struct {
    uint8_t x;
    uint8_t y;
} Pixel;

static Pixel snake[SNAKE_MAX_SIZE];
static uint16_t head = 0;
static uint16_t len = 1;

typedef enum { DIR_RIGHT, DIR_LEFT, DIR_UP, DIR_DOWN } Dir;
static Dir dir = DIR_RIGHT;         // Initial direction state of the snake (doesn't have to be right)

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
    snake[0].x = SNAKE_POSX;
    snake[0].y = SNAKE_POSY;
    head = 0;
    len = 1;
    dir = DIR_RIGHT;
    vga_draw_pixel(snake[0].x, snake[0].y, VGA_COLOR_BLUE);

    // Fruit
    vga_draw_pixel(FRUIT_POSX, FRUIT_POSY, VGA_COLOR_RED);
}

int main(void) {
    init_game();

    GameState state = WAIT_START;
    uint16_t prev_sw = 0;
    uint8_t prev_btn = 0;

    uint8_t score_count = 0;

    while (1) {
        uint16_t sw = read_switches();
        uint8_t sw0 = (sw & SW0_MASK) ? 1 : 0;
        uint8_t prev_sw0 = (prev_sw & SW0_MASK) ? 1 : 0;
        prev_sw = sw;

        // Wait for the switch
        if (state == WAIT_START) {
            if (sw0 && !prev_sw0) {
                count_down(COUNT_DOWN_START);
                state = PLAYING;
            }
        } 
        else { // PLAYING  
            
            uint8_t btn = read_buttons();

            // detect new button presses only (rising edge)
            uint8_t pressed = btn & (uint8_t)~prev_btn;
            prev_btn = btn;

            // prevent reverse direction
            if ((pressed & BTN_R_MASK) && dir != DIR_LEFT)
                dir = DIR_RIGHT;
            else if ((pressed & BTN_L_MASK) && dir != DIR_RIGHT)
                dir = DIR_LEFT;
            else if ((pressed & BTN_U_MASK) && dir != DIR_DOWN)
                dir = DIR_UP;
            else if ((pressed & BTN_D_MASK) && dir != DIR_UP)
                dir = DIR_DOWN;

            // compute new head position from current head
            uint8_t new_x = snake[head].x;
            uint8_t new_y = snake[head].y;

            // update new head position based on direction
            if (dir == DIR_RIGHT) new_x += 1;
            else if (dir == DIR_LEFT) new_x -= 1;
            else if (dir == DIR_UP) new_y -= 1;
            else if (dir == DIR_DOWN) new_y += 1;

            // border detection (screen is 128x64)
            if (new_x >= 128 || new_y >= 64) {
                state = WAIT_START;     // reset state
                init_game();            // redraw initial game
                continue;               // restart loop
            }

            // erase tail (only if not growing)
            uint16_t tail = (head + SNAKE_MAX_SIZE - (len - 1)) % SNAKE_MAX_SIZE;
            vga_draw_pixel(snake[tail].x, snake[tail].y, VGA_COLOR_GREEN);

            // advance head + store new head
            head = (head + 1) % SNAKE_MAX_SIZE;
            snake[head].x = new_x;
            snake[head].y = new_y;

            // draw new head
            vga_draw_pixel(new_x, new_y, VGA_COLOR_BLUE);

            // delay for better playable
            delay_ms(120);
        }
    }
    return 0;
}

