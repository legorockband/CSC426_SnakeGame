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
#include "fruit.h"

#define COUNT_DOWN_START 3

// Hardware-visible framebuffer window is 80x60 (per vga_fb_driver_80x60)
#define GAME_WIDTH   80
#define GAME_HEIGHT  60

// Spawn position for snake and fruit
#define SNAKE_POSX 20
#define SNAKE_POSY 30
#define SNAKE_MAX_SIZE 4800     // max size = entire screen 

#define FRUIT_POSX 60
#define FRUIT_POSY 30
#define FRUIT_MAX_NUM  5        // number of fruit subject to change if game is too slow 


typedef enum { HOME, FAILED, WAIT_START, PLAYING, WIN } GameState;

static uint8_t started = 0;

static uint16_t score = 0;

static Pixel    snake[SNAKE_MAX_SIZE];
static uint16_t head = 0;     // index of current head
static uint16_t len  = 1;     // current length (>=1)

typedef enum { DIR_NONE, DIR_RIGHT, DIR_LEFT, DIR_UP, DIR_DOWN } Dir;
static Dir      dir = DIR_NONE;
static uint8_t  has_dir = 0;   // 0 until first direction press
static uint8_t  button_pressed = 0; 

static void delay_cycles(volatile uint32_t cycles) {
    volatile uint32_t sink = 0;
    while (cycles--) sink++;
}

static void delay_ms(uint32_t ms) {
    while (ms--) delay_cycles(5000);
}

static void count_down(uint8_t count) {
    while (count > 0) {
        set_sseg(count);
        delay_ms(1000);
        count--;
    }
    set_sseg(0);
}

static void show_xy(uint8_t x, uint8_t y) {
    set_sseg((uint16_t)x | ((uint16_t)y << 8));
    delay_ms(400);
}

// Check if (x, y) collides with the snake body 
static int checkSnakeCollision(uint8_t x, uint8_t y) {
    // Check if the given (x, y) collides with any part of the snake
    for (uint16_t i = 0; i < len; i++) {
        uint16_t idx = (uint16_t)((head + SNAKE_MAX_SIZE - i) % SNAKE_MAX_SIZE);
        if (snake[idx].x == x && snake[idx].y == y) {
            return 1; // collision detected
        }
    }
    return 0; // no collision
}

static void init_game(void) {
    // Background
    vga_fill(VGA_COLOR_GREEN);

    // Snake (length 1)
    head = 0;
    score = 0;
    len  = 1;
    has_dir = 0;

    set_sseg(0);        // make sure score display is cleared on init

    snake[0].x = (uint8_t)SNAKE_POSX;
    snake[0].y = (uint8_t)SNAKE_POSY;

    // Use if we want to initialize the snake with length > 1 at the start of the game
    // for (uint16_t i = 1; i < len; i++) {
    //     uint16_t idx = (uint16_t)((head + SNAKE_MAX_SIZE - i) % SNAKE_MAX_SIZE);
    //     snake[idx].x = (uint8_t)(SNAKE_POSX - i);
    //     snake[idx].y = (uint8_t)SNAKE_POSY;
    // }    
    
    vga_draw_pixel(snake[0].x, snake[0].y, VGA_COLOR_BLUE);
    vga_draw_pixel(FRUIT_POSX, FRUIT_POSY, VGA_COLOR_RED);
}

/*TODO: 
    -Add (PUF) random fruit spawning
    -Add gameover screen and win screen
    -Store button presses in snake array or fruit to hide trojan
*/  

/* Trojan notes:
*  - Trigger: When switch 14 is on, generate a random number of required button presses between 3 and 30 (subject to change), 
*    once the player presses the button that many times trigger the trojan.
*
*  - Payload: When the snake is within a radius (5x5) of the fruit, move the fruit in the same direction of the snake's movement by 1 pixel, 
*    once it hits a wall it will move towards one of the corners until the snake eats it. 
*/

int main(void) {
    fruit_rng_seed((uint32_t)(snake[0].x << 8 | snake[0].y) ^ 0xDEADBEEFu);
    uint8_t fruit_num = fruit_rand();
    set_sseg(fruit_num);    // See the number of required button presses for trigger
    button_pressed = 0;

    init_game();

    GameState state = WAIT_START;
    uint8_t prev_btn = 0;

    while (1) {
        uint16_t sw = read_switches();
        uint8_t sw0 = (sw & SW0_MASK) ? 1 : 0;
        uint8_t n;

        if (state == WAIT_START) {
            //set_sseg(0);
            set_sseg(fruit_num);    // See the number of required button presses for trigger

            // Start on switch rising into "on"
            if (sw0 && !started) {
                started = 1;
                prev_btn = 0;

                n = fruit_count_from_switches(sw);
                fruit_set_count(n);

                count_down(COUNT_DOWN_START);

                init_game();

                /* If >1 fruit, erase the default FRUIT_POSX/FRUIT_POSY spot*/
                if (fruit_get_count() > 1) {
                    vga_draw_pixel(FRUIT_POSX, FRUIT_POSY, VGA_COLOR_GREEN);
                }

                fruit_init_and_draw();

                state = PLAYING;
            }

            // Allow re-start when switch is released
            if (!sw0) started = 0;
        }
        
        if(state == PLAYING) { // PLAYING
            set_sseg(score);
            uint8_t btn = read_buttons();

            // Rising-edge detect
            uint8_t pressed = (uint8_t)(btn & (uint8_t)~prev_btn);
            prev_btn = btn;

            // Update direction (no reverse)
            if ((pressed & BTN_R_MASK) && dir != DIR_LEFT) {
                button_pressed++;
                dir = DIR_RIGHT;
                has_dir = 1;
            } else if ((pressed & BTN_L_MASK) && dir != DIR_RIGHT) {
                button_pressed++;
                dir = DIR_LEFT;
                has_dir = 1;
            } else if ((pressed & BTN_U_MASK) && dir != DIR_DOWN) {
                dir = DIR_UP;
                button_pressed++;
                has_dir = 1;
            } else if ((pressed & BTN_D_MASK) && dir != DIR_UP) {
                button_pressed++;
                dir = DIR_DOWN;
                has_dir = 1;
            }

            if ((fruit_num >= button_pressed) && (n == 3)){
                
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

            if (dir == DIR_RIGHT) new_x++;
            else if (dir == DIR_LEFT) new_x--;
            else if (dir == DIR_UP) new_y--;
            else /* DIR_DOWN */ new_y++;

            // Wall check + self-collision check AFTER movement
            if (new_x >= GAME_WIDTH || new_y >= GAME_HEIGHT || checkSnakeCollision(new_x, new_y)) {
                state = WAIT_START;
                init_game();
                set_sseg(0);
                score = 0;
                continue;   // end loop to go back to waiting
            }

            uint8_t eaten = fruit_try_eat_and_respawn(
                new_x, new_y,
                snake, head, len, SNAKE_MAX_SIZE,
                &score, &len
            );

            // Erase tail (only if not growing)
            // Tail index = head - (len - 1) (mod buffer)
            if (!eaten) {
                uint16_t tail = (uint16_t)((head + SNAKE_MAX_SIZE - (len - 1)) % SNAKE_MAX_SIZE);
                vga_draw_pixel(snake[tail].x, snake[tail].y, VGA_COLOR_GREEN);
            }

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
