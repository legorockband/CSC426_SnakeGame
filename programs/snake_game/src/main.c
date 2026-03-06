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

// Hardware-visible framebuffer window is 80x60 (per vga_fb_driver_80x60)
#define GAME_WIDTH   80
#define GAME_HEIGHT  60

// Spawn position for snake and fruit
#define SNAKE_POSX 20
#define SNAKE_POSY 30
#define SNAKE_MAX_SIZE 4800     // max size = entire screen 

#define FRUIT_POSX 60
#define FRUIT_POSY 30
#define FRUIT_MAX_NUM  5        // subject to change if we want more fruit on the screen at once


typedef enum { FAILED, WAIT_START, PLAYING, WIN } GameState;

typedef struct {
    uint8_t x;
    uint8_t y;
} Pixel;

static uint8_t started = 0;

static uint16_t score = 0;

static Pixel    snake[SNAKE_MAX_SIZE];
static uint16_t head = 0;     // index of current head
static uint16_t len  = 1;     // current length (>=1)

static uint8_t  num_fruit = 1;      // number of fruit currently on the screen 
static uint8_t prev_num_fruit = 1;  // to track changes in number of fruit from switches and respawn if needed
static Pixel    fruit[FRUIT_MAX_NUM];

typedef enum { DIR_RIGHT, DIR_LEFT, DIR_UP, DIR_DOWN } Dir;
static Dir      dir = DIR_RIGHT;
static uint8_t  has_dir = 0;   // 0 until first direction press

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

/* Read the number of fruit to spawn from the switches
*  SW11 is 6, SW15 is 2, 
*/ 
static uint8_t get_num_fruit_from_switches(uint16_t sw) {
    // if (sw & SW11_MASK) return 6;
    if (sw & SW12_MASK) return 5;
    if (sw & SW13_MASK) return 4;
    if (sw & SW14_MASK) return 3;
    if (sw & SW15_MASK) return 2;
    return 1;
}

// Used for randomness for now
static uint32_t _rng_state = 0xC0FFEEu; /* seed; choose any non-zero value */

static void rng_seed(uint32_t seed) {
    if (seed == 0) seed = 0xC0FFEEu;
    _rng_state = seed;
}

static uint32_t rng_rand(void) {
    /* xorshift32 */
    uint32_t x = _rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    _rng_state = x ? x : 0x9u;
    return x;
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

static int generateValidFruitForIndex(uint16_t index) {
    if (index >= FRUIT_MAX_NUM) return 0;
    // guard to avoid infinite loop on crowded boards 
    const int MAX_ATTEMPTS = GAME_HEIGHT * GAME_WIDTH - len - num_fruit; 
    int attempts = 0;

    while (attempts++ < MAX_ATTEMPTS) {
        uint8_t rx = (uint8_t)(1 + (rng_rand() % (GAME_WIDTH  - 2)));
        uint8_t ry = (uint8_t)(1 + (rng_rand() % (GAME_HEIGHT - 2)));

        if (rx >= GAME_WIDTH || ry >= GAME_HEIGHT) continue;  // safety

        /* must not overlap snake or another fruit (except if replacing same index) */
        if (checkSnakeCollision(rx, ry)) continue;

        /* avoid overlapping other fruit indices */
        int dup = 0;
        for (uint16_t j = 0; j < num_fruit; j++) {
            if (j == index) continue;
            if (fruit[j].x == rx && fruit[j].y == ry) { dup = 1; break; }
        }
        if (dup) continue;

        /* found valid spot */
        fruit[index].x = rx;
        fruit[index].y = ry;
        return 1;
    }

    return 0; /* failed to find after many attempts */
}

static void drawFruit(void) {
    for (uint16_t i = 0; i < num_fruit; i++) {
        vga_draw_pixel(fruit[i].x, fruit[i].y, VGA_COLOR_RED);
    }
}

static void initFruits() {
    switch(num_fruit) {
        case 1:
            fruit[0].x = (uint8_t)FRUIT_POSX;
            fruit[0].y = (uint8_t)FRUIT_POSY;
            break;
        case 2:
            fruit[0].x = (uint8_t)(FRUIT_POSX - 5);
            fruit[0].y = (uint8_t)FRUIT_POSY;
            fruit[1].x = (uint8_t)(FRUIT_POSX + 5);
            fruit[1].y = (uint8_t)(FRUIT_POSY);
            break;
        case 3:
            // top of triangle
            fruit[0].x = (uint8_t)(FRUIT_POSX);
            fruit[0].y = (uint8_t)(FRUIT_POSY - 3);  
            // bottom left of triangle    
            fruit[1].x = (uint8_t)(FRUIT_POSX - 3);
            fruit[1].y = (uint8_t)(FRUIT_POSY + 3);
            // bottom right of triangle     
            fruit[2].x = (uint8_t)(FRUIT_POSX + 3);
            fruit[2].y = (uint8_t)(FRUIT_POSY + 3);     
            break;
        case 4: 
            // top left of rectangle
            fruit[0].x = (uint8_t)(FRUIT_POSX - 3);
            fruit[0].y = (uint8_t)(FRUIT_POSY - 3);
            // top right of rectangle      
            fruit[1].x = (uint8_t)(FRUIT_POSX + 3);
            fruit[1].y = (uint8_t)(FRUIT_POSY - 3); 
            // bottom left of rectangle    
            fruit[2].x = (uint8_t)(FRUIT_POSX - 3);
            fruit[2].y = (uint8_t)(FRUIT_POSY + 3); 
            // bottom right of rectangle    
            fruit[3].x = (uint8_t)(FRUIT_POSX + 3);
            fruit[3].y = (uint8_t)(FRUIT_POSY + 3);     
            break;
        case 5:
            // center
            fruit[0].x = (uint8_t)(FRUIT_POSX);
            fruit[0].y = (uint8_t)(FRUIT_POSY);
            // top left of rectangle
            fruit[1].x = (uint8_t)(FRUIT_POSX - 3);
            fruit[1].y = (uint8_t)(FRUIT_POSY - 3);
            // top right of rectangle      
            fruit[2].x = (uint8_t)(FRUIT_POSX + 3);
            fruit[2].y = (uint8_t)(FRUIT_POSY - 3); 
            // bottom left of rectangle    
            fruit[3].x = (uint8_t)(FRUIT_POSX - 3);
            fruit[3].y = (uint8_t)(FRUIT_POSY + 3); 
            // bottom right of rectangle    
            fruit[4].x = (uint8_t)(FRUIT_POSX + 3);
            fruit[4].y = (uint8_t)(FRUIT_POSY + 3);  
            break;
        // Add more cases as needed
    }
    drawFruit();
}

static void setFruitPosition(uint8_t index, uint8_t x, uint8_t y) { 
    if (index < FRUIT_MAX_NUM) { 
        fruit[index].x = x; fruit[index].y = y; 
    } 
}

static int fruitCollisionCheck(uint8_t x, uint8_t y) {
    for (uint16_t i = 0; i < num_fruit; i++) {
        if (fruit[i].x == x && fruit[i].y == y) {
            // Handle fruit collision (e.g., increase score, grow snake, respawn fruit)
            score++;
            if (len < SNAKE_MAX_SIZE) len++;

            if(!generateValidFruitForIndex(i)) {
                setFruitPosition(i, FRUIT_POSX, FRUIT_POSY); // fallback to default position if failed to generate new fruit
            }
            else{
                vga_draw_pixel(fruit[i].x, fruit[i].y, VGA_COLOR_RED);
            }
            return 1;   // eaten
            
        }
    }
    return 0; // not eaten
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

    fruit[0].x = (uint8_t)FRUIT_POSX;
    fruit[0].y = (uint8_t)FRUIT_POSY;

    // Use if we want to initialize the snake with length > 1 at the start of the game
    // for (uint16_t i = 1; i < len; i++) {
    //     uint16_t idx = (uint16_t)((head + SNAKE_MAX_SIZE - i) % SNAKE_MAX_SIZE);
    //     snake[idx].x = (uint8_t)(SNAKE_POSX - i);
    //     snake[idx].y = (uint8_t)SNAKE_POSY;
    // }    
    
    vga_draw_pixel(snake[0].x, snake[0].y, VGA_COLOR_BLUE);
    vga_draw_pixel(fruit[0].x, fruit[0].y, VGA_COLOR_RED);
}

/*TODO: 
    -Add (PUF) random fruit spawning
    -Add gameover screen and win screen
    -Store button presses in snake array or fruit to hide trojan
*/     

int main(void) {
    rng_seed( (uint32_t) (snake[0].x << 8 | snake[0].y) ^ 0xDEADBEEFu );
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

                num_fruit = get_num_fruit_from_switches(sw);

                count_down(COUNT_DOWN_START);
                
                init_game();
                if (num_fruit > 1){
                    vga_draw_pixel(FRUIT_POSX, FRUIT_POSY, VGA_COLOR_GREEN); // erase default fruit if more than 1 fruit selected
                }
                initFruits();

                state = PLAYING;
            }

            // Allow re-start when switch is released
            if (!sw0) started = 0;
        }
        
        else { // PLAYING
            set_sseg(score);
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

            if (dir == DIR_RIGHT) new_x++;
            else if (dir == DIR_LEFT) new_x--;
            else if (dir == DIR_UP) new_y--;
            else /* DIR_DOWN */ new_y++;

            // Wall check + self-collision check AFTER movement
            if (1 > new_x || new_x >= GAME_WIDTH || 0 > new_y || new_y >= GAME_HEIGHT || checkSnakeCollision(new_x, new_y)) {
                state = WAIT_START;
                init_game();
                set_sseg(0);
                score = 0;
                continue;   // end loop to go back to waiting
            }

            uint8_t eaten = fruitCollisionCheck(new_x, new_y);

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
