#include "fruit.h"

#include "vga.h"
#include "outputs.h"  /* for set_led() */

#define GAME_WIDTH   80
#define GAME_HEIGHT  60

#define FRUIT_POSX 60
#define FRUIT_POSY 30

#define FRUIT_MAX_NUM  10

static Pixel   fruit[FRUIT_MAX_NUM];
static uint8_t num_fruit = 1;

/* ---------- RNG (xorshift32) ---------- */
static uint32_t _rng_state = 0xC0FFEEu;

void fruit_rng_seed(uint32_t seed) {
    if (seed == 0) seed = 0xC0FFEEu;
    _rng_state = seed;
}

static uint32_t rng_rand(void) {
    uint32_t x = _rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    _rng_state = x ? x : 0x9u;
    return x;
}

uint8_t fruit_rand(void){
    return (uint8_t)(3u + (rng_rand() % 28u)); // 3..30
}

/* ---------- Public count helpers ---------- */
void fruit_set_count(uint8_t n) {
    if (n < 1) n = 1;
    if (n > 5) n = 5; /* your game logic uses 1..5 */
    num_fruit = n;
}

uint8_t fruit_get_count(void) {
    return num_fruit;
}

// Make a radius around the fruit of 5x5 and return the pixels in that radius
static uint8_t u8_abs_diff(uint8_t a, uint8_t b) {
    return (a > b) ? (uint8_t)(a - b) : (uint8_t)(b - a);
}

static uint8_t in_7x7_box(uint8_t sx, uint8_t sy, uint8_t fx, uint8_t fy) {
    return (u8_abs_diff(sx, fx) <= 3) && (u8_abs_diff(sy, fy) <= 3);
}

/* Same mapping you had (SW12->5, SW13->4, SW14->3, SW15->2, else 1) */
uint8_t fruit_count_from_switches(uint16_t sw) {
    /* If your project defines SW11..SW15 masks, keep them here. */
    if (sw & SW12_MASK) return 5;
    if (sw & SW13_MASK) return 4;
    if (sw & SW14_MASK) return 3;
    if (sw & SW15_MASK) return 2;
    return 1;
}

/* ---------- Drawing ---------- */
void fruit_draw_all(void) {
    for (uint16_t i = 0; i < num_fruit; i++) {
        vga_draw_pixel(fruit[i].x, fruit[i].y, VGA_COLOR_RED);
    }
}

/* Deterministic “starting shapes” you had */
void fruit_init_and_draw(void) {
    /* Clear only the default fruit if you want is main.c’s job,
       but it’s harmless if the caller already cleared it. */

    switch (num_fruit) {
        case 2:
            fruit[0].x = (uint8_t)(FRUIT_POSX - 5);
            fruit[0].y = (uint8_t)(FRUIT_POSY);
            fruit[1].x = (uint8_t)(FRUIT_POSX + 5);
            fruit[1].y = (uint8_t)(FRUIT_POSY);
            break;

        case 3:
            fruit[0].x = (uint8_t)(FRUIT_POSX);
            fruit[0].y = (uint8_t)(FRUIT_POSY - 3);
            fruit[1].x = (uint8_t)(FRUIT_POSX - 3);
            fruit[1].y = (uint8_t)(FRUIT_POSY + 3);
            fruit[2].x = (uint8_t)(FRUIT_POSX + 3);
            fruit[2].y = (uint8_t)(FRUIT_POSY + 3);
            break;

        case 4:
            fruit[0].x = (uint8_t)(FRUIT_POSX - 3);
            fruit[0].y = (uint8_t)(FRUIT_POSY - 3);
            fruit[1].x = (uint8_t)(FRUIT_POSX + 3);
            fruit[1].y = (uint8_t)(FRUIT_POSY - 3);
            fruit[2].x = (uint8_t)(FRUIT_POSX - 3);
            fruit[2].y = (uint8_t)(FRUIT_POSY + 3);
            fruit[3].x = (uint8_t)(FRUIT_POSX + 3);
            fruit[3].y = (uint8_t)(FRUIT_POSY + 3);
            break;

        case 5:
            fruit[0].x = (uint8_t)(FRUIT_POSX);
            fruit[0].y = (uint8_t)(FRUIT_POSY);
            fruit[1].x = (uint8_t)(FRUIT_POSX - 3);
            fruit[1].y = (uint8_t)(FRUIT_POSY - 3);
            fruit[2].x = (uint8_t)(FRUIT_POSX + 3);
            fruit[2].y = (uint8_t)(FRUIT_POSY - 3);
            fruit[3].x = (uint8_t)(FRUIT_POSX - 3);
            fruit[3].y = (uint8_t)(FRUIT_POSY + 3);
            fruit[4].x = (uint8_t)(FRUIT_POSX + 3);
            fruit[4].y = (uint8_t)(FRUIT_POSY + 3);
            break;

        default:
            fruit[0].x = (uint8_t)FRUIT_POSX;
            fruit[0].y = (uint8_t)FRUIT_POSY;
            break;
    }

    fruit_draw_all();
}

/* ---------- Internal helpers for valid respawn ---------- */

static uint8_t snake_collides(
    uint8_t x, uint8_t y, Pixel* snake,
    uint16_t head, uint16_t len, uint16_t snake_max_size) {
    for (uint16_t i = 0; i < len; i++) {
        uint16_t idx = (uint16_t)((head + snake_max_size - i) % snake_max_size);
        if (snake[idx].x == x && snake[idx].y == y) return 1;
    }
    return 0;
}

static uint8_t generate_valid_fruit_for_index(
    uint16_t index,Pixel* snake, uint16_t head, uint16_t len,
    uint16_t snake_max_size) {
    if (index >= FRUIT_MAX_NUM) return 0;

    /* guard to avoid infinite loop on crowded boards */
    const int MAX_ATTEMPTS = (GAME_HEIGHT * GAME_WIDTH);
    int attempts = 0;

    while (attempts++ < MAX_ATTEMPTS) {
        uint8_t rx = (uint8_t)(1 + (rng_rand() % (GAME_WIDTH  - 2)));
        uint8_t ry = (uint8_t)(1 + (rng_rand() % (GAME_HEIGHT - 2)));

        if (rx >= GAME_WIDTH || ry >= GAME_HEIGHT) continue;

        if (snake_collides(rx, ry, snake, head, len, snake_max_size)) continue;

        /* avoid overlapping other fruits */
        uint8_t dup = 0;
        for (uint16_t j = 0; j < num_fruit; j++) {
            if (j == index) continue;
            if (fruit[j].x == rx && fruit[j].y == ry) { dup = 1; break; }
        }
        if (dup) continue;

        fruit[index].x = rx;
        fruit[index].y = ry;
        return 1;
    }

    return 0;
}

/* ---------- Eat + respawn ---------- */
uint8_t fruit_try_eat_and_respawn(
    uint8_t new_x, uint8_t new_y,
    Pixel*  snake, uint16_t head, uint16_t len, uint16_t snake_max_size,
    uint16_t* score,uint16_t* len_inout) {
    uint8_t any_near = 0;
    for (uint16_t i = 0; i < num_fruit; i++) {
        if (fruit[i].x == new_x && fruit[i].y == new_y) {
            /* eaten */
            if (score) (*score)++;
            if (len_inout && (*len_inout) < snake_max_size) (*len_inout)++;

            if (!generate_valid_fruit_for_index(i, snake, head, len, snake_max_size)) {
                fruit[i].x = (uint8_t)FRUIT_POSX;
                fruit[i].y = (uint8_t)FRUIT_POSY;
            }

            vga_draw_pixel(fruit[i].x, fruit[i].y, VGA_COLOR_RED);
            set_leds(0);
            return 1;
        }

        if (!any_near && in_7x7_box(new_x, new_y, fruit[i].x, fruit[i].y)) {
            any_near = 1;
        }
    }
    set_leds(any_near ? 1 : 0);
    return 0;
}