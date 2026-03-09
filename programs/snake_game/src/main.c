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

// Hardware-visible framebuffer window is 80x60 
#define GAME_WIDTH   80
#define GAME_HEIGHT  60

// Spawn position for snake and fruit
#define SNAKE_POSX 20
#define SNAKE_POSY 30
#define SNAKE_MAX_SIZE 4800     // entire screen      

typedef enum { HOME, PLAYING, WIN, EXIT_SCREEN } GameState;
typedef enum { MENU_START, MENU_EXIT } MenuChoice;
typedef enum { CONFIRM_RISING, CONFIRM_FALLING } ConfirmEdge;

static MenuChoice home_choice = MENU_START;

static uint8_t  confirm_armed = 0;
static uint16_t high_score = 0;
static uint16_t score = 0;

static uint8_t  respawn = 0;
static uint8_t  started = 0;
static uint32_t entropy = 0;            // rng seed helper

static Pixel    snake[SNAKE_MAX_SIZE];
static uint16_t head = 0;               // index of  head
static uint16_t len  = 1;               // length of snake

static Dir      dir = DIR_NONE;
static uint8_t  has_dir = 0;            // 0 until first direction press
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
    } 
    else { // CONFIRM_FALLING
        triggered = (!sw_now && *sw_prev);
    }

    *sw_prev = sw_now;
    return triggered;
}

static void arm_confirm_for_edge(uint8_t sw_now, ConfirmEdge edge) {
    if (edge == CONFIRM_RISING) {
        confirm_armed = (sw_now == 0);
    } 
    else { // CONFIRM_FALLING
        confirm_armed = (sw_now == 1);
    }
}

static uint8_t confirm_ready_event(uint8_t sw_now, uint8_t *sw_prev, ConfirmEdge edge) {
    uint8_t event = confirm_event(sw_now, sw_prev, edge);

    if (!confirm_armed) {
        if (edge == CONFIRM_RISING && sw_now == 0) {
            confirm_armed = 1;
        } 
        else if (edge == CONFIRM_FALLING && sw_now == 1) {
            confirm_armed = 1;
        }
    }

    if (confirm_armed && event) {
        confirm_armed = 0;
        return 1;
    }

    return 0;
}

static inline void rng_mix_inputs(uint16_t sw, uint8_t btn, uint32_t tag) {
    entropy += 0x9E3779B9u;                 // advance counter
    uint32_t v = ((uint32_t)sw << 16) ^ ((uint32_t)btn << 8) ^ entropy ^ tag;
    fruit_rng_stir(v);                 // stir fruit RNG only
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

    // Snake attributes 
    head = 0;
    score = 0;
    len  = 1;
    has_dir = 0;
    dir = DIR_NONE;
    respawn = 0;

    button_pressed = 0;

    set_sseg(0);        // make sure score display is cleared on init

    snake[0].x = (uint8_t)SNAKE_POSX;
    snake[0].y = (uint8_t)SNAKE_POSY;
    
    vga_draw_pixel(snake[0].x, snake[0].y, VGA_COLOR_BLUE);
    vga_draw_pixel(FRUIT_POSX, FRUIT_POSY, VGA_COLOR_RED);
}

int main(void) {
    fruit_rng_seed(0xDEADBEEFu);
    uint8_t fruit_num = 1;
    button_pressed = 0;

    GameState state = HOME;
    uint8_t prev_btn = read_buttons();
    uint16_t prev_sw = read_switches(); 
    uint8_t prev_sw0 = (read_switches() & SW0_MASK) ? 1 : 0;

    arm_confirm_for_edge(prev_sw0, CONFIRM_RISING);
    draw_home_screen((home_choice == MENU_START), score, high_score);

    while (1) {
        uint16_t sw = read_switches();
        uint8_t sw0 = (sw & SW0_MASK) ? 1 : 0;
        uint8_t count = (sw & SW14_MASK) ? 1 : 0;

        if (state == HOME) {
            uint8_t  btn = read_buttons();
            uint8_t  pressed = (uint8_t)(btn & (uint8_t)~prev_btn);
            uint8_t  btn_changed = (uint8_t)(btn ^ prev_btn);

            uint16_t sw_changed = (uint16_t)(sw ^ prev_sw);

            if (sw_changed || btn_changed) {
                rng_mix_inputs(sw, btn, 0xC1A4u);
            }

            prev_btn = btn;
            prev_sw  = sw;

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

            // Start / Exit confirmed by SW0 rising edge
            if (confirm_ready_event(sw0, &prev_sw0, CONFIRM_RISING)) {
                if (home_choice == MENU_START) {
                    prev_btn = read_buttons();

                    started = 1;
                    button_pressed = 0;
                    respawn = 0;
                    fruit_num = fruit_rand();

                    uint8_t n = fruit_count_from_switches(sw);
                    fruit_set_count(n);

                    count_down(COUNT_DOWN_START);
                    init_game();

                    // If >1 fruit, erase the default fruit position spot
                    if (fruit_get_count() > 1) {
                        vga_draw_pixel(FRUIT_POSX, FRUIT_POSY, VGA_COLOR_GREEN);
                    }

                    fruit_init_and_draw();

                    state = PLAYING;
                    continue;
                } 
                else { // MENU_EXIT
                    state = EXIT_SCREEN;
                    arm_confirm_for_edge(sw0, CONFIRM_FALLING);
                    draw_exit_screen(high_score);
                    prev_btn = read_buttons();
                    continue;
                }
            }
        }
        
        if (state == PLAYING) { // PLAYING
            set_sseg(score);
            uint8_t btn = read_buttons();

            // Rising-edge detect
            uint8_t pressed = (uint8_t)(btn & (uint8_t)~prev_btn);
            prev_btn = btn;

            if (pressed){
                button_pressed++;
                entropy++;
                rng_mix_inputs(sw, pressed, 0x91A7u);            
            }

            // Update direction (no reverse)
            if ((pressed & BTN_R_MASK) && dir != DIR_LEFT) {
                dir = DIR_RIGHT;
                has_dir = 1;
            } 
            else if ((pressed & BTN_L_MASK) && dir != DIR_RIGHT) {
                dir = DIR_LEFT;
                has_dir = 1;
            } 
            else if ((pressed & BTN_U_MASK) && dir != DIR_DOWN) {
                dir = DIR_UP;
                has_dir = 1;
            } 
            else if ((pressed & BTN_D_MASK) && dir != DIR_UP) {
                dir = DIR_DOWN;
                has_dir = 1;
            }

            if (!respawn) {
                if ((fruit_num <= button_pressed) && count){
                    respawn = 1;
                }
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

            // Determine whether this move will grow the snake 
            uint8_t will_eat = fruit_is_at(new_x, new_y);

            uint16_t tail = (uint16_t)((head + SNAKE_MAX_SIZE - (len - 1)) % SNAKE_MAX_SIZE);
            uint8_t tail_x = snake[tail].x;
            uint8_t tail_y = snake[tail].y;

            uint8_t self_hit = checkSnakeCollision(new_x, new_y);
            if (!will_eat && new_x == tail_x && new_y == tail_y) {
                self_hit = 0;
            }

            if (new_x >= GAME_WIDTH || new_y >= GAME_HEIGHT || self_hit) {
                update_high_score();
                state = WIN;
                prev_sw0 = 1;
                arm_confirm_for_edge(sw0, CONFIRM_FALLING);
                draw_win_screen(score, high_score);
                prev_btn = read_buttons();
                continue;
            }

            uint8_t eaten = fruit_try_eat_and_respawn(
                cur_x, cur_y,
                new_x, new_y,
                snake, head, len, SNAKE_MAX_SIZE,
                &score, &len, dir, respawn
            );

            if (!eaten) {
                uint16_t tail = (uint16_t)((head + SNAKE_MAX_SIZE - (len - 1)) % SNAKE_MAX_SIZE);
                vga_draw_pixel(snake[tail].x, snake[tail].y, VGA_COLOR_GREEN);
            }

            head = (uint16_t)((head + 1) % SNAKE_MAX_SIZE);
            snake[head].x = new_x;
            snake[head].y = new_y;

            // Draw new head
            vga_draw_pixel(new_x, new_y, VGA_COLOR_BLUE);

            // Control game speed
            delay_ms(120);
        }

        else if (state == WIN) {
            set_sseg(score);

            if (!sw0) {
                home_choice = MENU_START;
                state = HOME;

                // now we want a rising edge to start again
                arm_confirm_for_edge(sw0, CONFIRM_RISING);
                draw_home_screen((home_choice == MENU_START), score, high_score);

                prev_btn = read_buttons();
                prev_sw0 = 0;   // keep the edge detector aligned
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

        if (!sw0) started = 0;
    }

    return 0;
}
