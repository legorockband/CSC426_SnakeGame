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

// Spawn position (must be within GAME bounds)
#define SNAKE_POSX 20
#define SNAKE_POSY 30

// Hardware-visible framebuffer window is 80x60 (per vga_fb_driver_80x60)
#define GAME_WIDTH   80
#define GAME_HEIGHT  60

// Keep the buffer size <= 65535; 256 is fine
#define SNAKE_MAX_SIZE 256

#define FRUIT_POSX 60
#define FRUIT_POSY 30

typedef enum { WAIT_START, PLAYING } GameState;

typedef struct {
    uint8_t x;
    uint8_t y;
} Pixel;

static uint8_t started = 0;

static Pixel   snake[SNAKE_MAX_SIZE];
static uint16_t head = 0;     // index of current head
static uint16_t len  = 1;     // current length (>=1)

typedef enum { DIR_RIGHT, DIR_LEFT, DIR_UP, DIR_DOWN } Dir;
static Dir     dir = DIR_RIGHT;
static uint8_t has_dir = 0;   // 0 until first direction press

static void delay_cycles(volatile uint32_t cycles)
{
    volatile uint32_t sink = 0;
    while (cycles--) sink++;
}

static void delay_ms(uint32_t ms)
{
    while (ms--) delay_cycles(5000);
}

static void count_down(uint8_t count)
{
    while (count > 0) {
        set_sseg(count);
        delay_ms(1000);
        count--;
    }
    set_sseg(0);
}

static void show_xy(uint8_t x, uint8_t y)
{
    set_sseg((uint16_t)x | ((uint16_t)y << 8));
    delay_ms(400);
}

static void init_game(void)
{
    // Background
    vga_fill(VGA_COLOR_GREEN);

    // Snake (length 1)
    head = 0;
    len  = 1;
    dir  = DIR_RIGHT;
    has_dir = 0;

    snake[0].x = (uint8_t)SNAKE_POSX;
    snake[0].y = (uint8_t)SNAKE_POSY;

    vga_draw_pixel(snake[0].x, snake[0].y, VGA_COLOR_BLUE);

    // Fruit (static for now)
    vga_draw_pixel(FRUIT_POSX, FRUIT_POSY, VGA_COLOR_RED);
}

int main(void)
{
    init_game();

    GameState state = WAIT_START;
    uint8_t prev_btn = 0;

    while (1) {
        uint16_t sw = read_switches();
        uint8_t sw0 = (sw & SW0_MASK) ? 1 : 0;

        if (state == WAIT_START) {
            set_sseg(0);

            // Start on switch rising into "on"
            if (sw0 && !started) {
                started = 1;
                prev_btn = 0;

                count_down(COUNT_DOWN_START);
                
                init_game();
                state = PLAYING;
            }

            // Allow re-start when switch is released
            if (!sw0) started = 0;
        }
        else { // PLAYING
            uint8_t btn = read_buttons();

            // Rising-edge detect
            uint8_t pressed = (uint8_t)(btn & (uint8_t)~prev_btn);
            prev_btn = btn;

            // Update direction (no reverse)
            if ((pressed & BTN_R_MASK) && dir != DIR_LEFT) {
                dir = DIR_RIGHT;
                has_dir = 1;
            } else if ((pressed & BTN_L_MASK) && dir != DIR_RIGHT) {
                dir = DIR_LEFT;
                has_dir = 1;
            } else if ((pressed & BTN_U_MASK) && dir != DIR_DOWN) {
                dir = DIR_UP;
                has_dir = 1;
            } else if ((pressed & BTN_D_MASK) && dir != DIR_UP) {
                dir = DIR_DOWN;
                has_dir = 1;
            }

            // Wait for first direction press
            if (!has_dir) {
                delay_ms(20);
                continue;
            }

            // Current head position
            uint8_t cur_x = snake[head].x;
            uint8_t cur_y = snake[head].y;

            // Compute next head position
            uint8_t new_x = cur_x;
            uint8_t new_y = cur_y;

            // If memory got corrupted, snap back to spawn instead of using (0,0)
            if (cur_x >= GAME_WIDTH || cur_y >= GAME_HEIGHT) {
                cur_x = SNAKE_POSX;
                cur_y = SNAKE_POSY;
                snake[head].x = cur_x;
                snake[head].y = cur_y;
            }

            if (dir == DIR_RIGHT) {
                if (new_x >= (GAME_WIDTH - 1)) { state = WAIT_START; init_game(); continue; }
                new_x++;
            } else if (dir == DIR_LEFT) {
                if (new_x == 0) { state = WAIT_START; init_game(); continue; }
                new_x--;
            } else if (dir == DIR_UP) {
                if (new_y == 0) { state = WAIT_START; init_game(); continue; }
                new_y--;
            } else { // DIR_DOWN
                if (new_y >= (GAME_HEIGHT - 1)) { state = WAIT_START; init_game(); continue; }
                new_y++;
            }

            // Erase tail (only if not growing)
            // Tail index = head - (len - 1) (mod buffer)
            uint16_t tail = (uint16_t)((head + SNAKE_MAX_SIZE - (len - 1)) % SNAKE_MAX_SIZE);
            vga_draw_pixel(snake[tail].x, snake[tail].y, VGA_COLOR_GREEN);

            // Advance head and write new head position
            head = (uint16_t)((head + 1) % SNAKE_MAX_SIZE);
            snake[head].x = new_x;
            snake[head].y = new_y;

            // Draw new head
            vga_draw_pixel(new_x, new_y, VGA_COLOR_BLUE);

            // Control game speed
            delay_ms(120);
        }
    }

    return 0;
}
