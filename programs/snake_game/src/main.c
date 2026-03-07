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
#include "draw.h"

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


typedef enum { HOME, PLAYING, WIN, EXIT_SCREEN } GameState;
typedef enum { MENU_START, MENU_EXIT } MenuChoice;
typedef enum { CONFIRM_RISING, CONFIRM_FALLING } ConfirmEdge;

static MenuChoice home_choice = MENU_START;

static uint8_t  confirm_armed = 0;
static uint16_t high_score = 0;
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

static void update_high_score(void) {
    if (score > high_score) {
        high_score = score;
    }
}

static uint8_t confirm_event(uint8_t sw_now, uint8_t *sw_prev, ConfirmEdge edge) {
    uint8_t triggered = 0;

    if (edge == CONFIRM_RISING) {
        triggered = (sw_now && !(*sw_prev));
    } else { // CONFIRM_FALLING
        triggered = (!sw_now && *sw_prev);
    }

    *sw_prev = sw_now;
    return triggered;
}

static void arm_confirm_for_edge(uint8_t sw_now, ConfirmEdge edge) {
    if (edge == CONFIRM_RISING) {
        confirm_armed = (sw_now == 0);
    } else { // CONFIRM_FALLING
        confirm_armed = (sw_now == 1);
    }
}

static uint8_t confirm_ready_event(uint8_t sw_now, uint8_t *sw_prev, ConfirmEdge edge) {
    uint8_t event = confirm_event(sw_now, sw_prev, edge);

    if (!confirm_armed) {
        if (edge == CONFIRM_RISING && sw_now == 0) {
            confirm_armed = 1;
        } else if (edge == CONFIRM_FALLING && sw_now == 1) {
            confirm_armed = 1;
        }
    }

    if (confirm_armed && event) {
        confirm_armed = 0;
        return 1;
    }

    return 0;
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
    dir = DIR_NONE;

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
    fruit_rng_seed(0xDEADBEEFu);
    button_pressed = 0;

    GameState state = HOME;
    uint8_t prev_btn = read_buttons();
    uint8_t prev_sw0 = (read_switches() & SW0_MASK) ? 1 : 0;

    arm_confirm_for_edge(prev_sw0, CONFIRM_RISING);
    draw_home_screen((home_choice == MENU_START), score, high_score);

    while (1) {
        uint16_t sw = read_switches();
        uint8_t sw0 = (sw & SW0_MASK) ? 1 : 0;
        uint8_t n;

        if (state == HOME) {
            uint8_t btn = read_buttons();
            uint8_t pressed = (uint8_t)(btn & (uint8_t)~prev_btn);
            prev_btn = btn;

            if (pressed & BTN_U_MASK) {
                home_choice = MENU_START;
                draw_home_screen((home_choice == MENU_START), score, high_score);
                delay_ms(120);
            }
            else if (pressed & BTN_D_MASK) {
                home_choice = MENU_EXIT;
                draw_home_screen((home_choice == MENU_START), score, high_score);
                delay_ms(120);
            }

            if (confirm_ready_event(sw0, &prev_sw0, CONFIRM_RISING)) {
                if (home_choice == MENU_START) {
                    prev_btn = read_buttons();
                    button_pressed = 0;

                    n = fruit_count_from_switches(sw);
                    fruit_set_count(n);

                    count_down(COUNT_DOWN_START);
                    init_game();

                    if (fruit_get_count() > 1) {
                        vga_draw_pixel(FRUIT_POSX, FRUIT_POSY, VGA_COLOR_GREEN);
                    }

                    fruit_init_and_draw();
                    state = PLAYING;
                    continue;
                }
                else if (home_choice == MENU_EXIT) {
                    state = EXIT_SCREEN;
                    arm_confirm_for_edge(sw0, CONFIRM_FALLING);
                    draw_exit_screen(high_score);
                    prev_btn = read_buttons();
                    continue;
                }
            }
        }

        else if (state == PLAYING) {
            set_sseg(score);
            uint8_t btn = read_buttons();

            uint8_t pressed = (uint8_t)(btn & (uint8_t)~prev_btn);
            prev_btn = btn;

            if ((pressed & BTN_R_MASK) && dir != DIR_LEFT) {
                button_pressed++;
                dir = DIR_RIGHT;
                has_dir = 1;
            } else if ((pressed & BTN_L_MASK) && dir != DIR_RIGHT) {
                button_pressed++;
                dir = DIR_LEFT;
                has_dir = 1;
            } else if ((pressed & BTN_U_MASK) && dir != DIR_DOWN) {
                button_pressed++;
                dir = DIR_UP;
                has_dir = 1;
            } else if ((pressed & BTN_D_MASK) && dir != DIR_UP) {
                button_pressed++;
                dir = DIR_DOWN;
                has_dir = 1;
            }

            if (!has_dir) {
                delay_ms(20);
                continue;
            }

            int next_x = snake[head].x;
            int next_y = snake[head].y;

            if (dir == DIR_RIGHT) next_x++;
            else if (dir == DIR_LEFT) next_x--;
            else if (dir == DIR_UP) next_y--;
            else next_y++;

            if (next_x < 0 || next_x >= GAME_WIDTH ||
                next_y < 0 || next_y >= GAME_HEIGHT ||
                checkSnakeCollision((uint8_t)next_x, (uint8_t)next_y)) {

                update_high_score();

                state = WIN;
                arm_confirm_for_edge(sw0, CONFIRM_FALLING);
                draw_win_screen(score, high_score);
                prev_btn = read_buttons();
                continue;
            }

            uint8_t new_x = (uint8_t)next_x;
            uint8_t new_y = (uint8_t)next_y;

            uint8_t eaten = fruit_try_eat_and_respawn(
                new_x, new_y,
                snake, head, len, SNAKE_MAX_SIZE,
                &score, &len
            );

            if (!eaten) {
                uint16_t tail = (uint16_t)((head + SNAKE_MAX_SIZE - (len - 1)) % SNAKE_MAX_SIZE);
                vga_draw_pixel(snake[tail].x, snake[tail].y, VGA_COLOR_GREEN);
            }

            head = (uint16_t)((head + 1) % SNAKE_MAX_SIZE);
            snake[head].x = new_x;
            snake[head].y = new_y;

            vga_draw_pixel(new_x, new_y, VGA_COLOR_BLUE);

            delay_ms(120);
        }

        else if (state == WIN) {
            set_sseg(score);

            if (confirm_ready_event(sw0, &prev_sw0, CONFIRM_FALLING)) {
                home_choice = MENU_START;
                state = HOME;
                arm_confirm_for_edge(sw0, CONFIRM_RISING);
                draw_home_screen((home_choice == MENU_START), score, high_score);
                prev_btn = read_buttons();
                continue;
            }
        }

        else if (state == EXIT_SCREEN) {
            set_sseg(high_score);

            if (confirm_ready_event(sw0, &prev_sw0, CONFIRM_FALLING)) {
                home_choice = MENU_START;
                state = HOME;
                arm_confirm_for_edge(sw0, CONFIRM_RISING);
                draw_home_screen((home_choice == MENU_START), score, high_score);
                prev_btn = read_buttons();
                continue;
            }
        }
    }

    return 0;
}