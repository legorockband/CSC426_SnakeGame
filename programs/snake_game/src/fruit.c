#include "fruit.h"

#include "vga.h"
#include "outputs.h"  /* for set_led() */

#define GAME_WIDTH   80
#define GAME_HEIGHT  60

#define FRUIT_POSX 60
#define FRUIT_POSY 30

static Pixel   fruit[FRUIT_MAX_NUM];
static uint8_t other_fruit[FRUIT_MAX_NUM] = {0};
static uint8_t num_fruit = 1;

static uint8_t is_corner(uint8_t x, uint8_t y) {
    uint8_t left   = (x == 0);
    uint8_t right  = (x == (GAME_WIDTH  - 1));
    uint8_t top    = (y == 0);
    uint8_t bottom = (y == (GAME_HEIGHT - 1));
    return (left || right) && (top || bottom);
}

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

void fruit_rng_stir(uint32_t v) {
    /* cheap mixing (similar spirit to boost/hash combine) */
    _rng_state ^= v + 0x9E3779B9u + (_rng_state << 6) + (_rng_state >> 2);
    (void)rng_rand(); /* advance once to diffuse */
}

uint8_t fruit_rand(void){
    return (uint8_t)(3u + (rng_rand() % 48u)); 
}

/* ---------- Public count helpers ---------- */
void fruit_set_count(uint8_t n) {
    if (n < 1) n = 1;
    if (n > 5) n = 5; 
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

static uint8_t manhattan(uint8_t ax, uint8_t ay, uint8_t bx, uint8_t by) {
    return (uint8_t)(u8_abs_diff(ax, bx) + u8_abs_diff(ay, by));
}

static uint8_t moving_toward(
    uint8_t cur_x, uint8_t cur_y,
    uint8_t new_x, uint8_t new_y,
    uint8_t fx, uint8_t fy
) {
    uint8_t before = manhattan(cur_x, cur_y, fx, fy);
    uint8_t after  = manhattan(new_x, new_y, fx, fy);
    return (after < before);
}

static int16_t fruit_index_at(uint8_t x, uint8_t y) {
    for (uint16_t j = 0; j < num_fruit; j++) {
        if (fruit[j].x == x && fruit[j].y == y) return (int16_t)j;
    }
    return -1;
}

static uint8_t snake_collides(
    uint8_t x, uint8_t y, Pixel* snake,
    uint16_t head, uint16_t len, uint16_t snake_max_size) {
    for (uint16_t i = 0; i < len; i++) {
        uint16_t idx = (uint16_t)((head + snake_max_size - i) % snake_max_size);
        if (snake[idx].x == x && snake[idx].y == y) return 1;
    }
    return 0;
}

static uint8_t can_place_fruit_here(
    uint16_t self, uint8_t x, uint8_t y,
    Pixel* snake, uint16_t head, uint16_t len, uint16_t snake_max_size
) {
    // in-bounds already guaranteed by caller
    if (snake_collides(x, y, snake, head, len, snake_max_size)) return 0;

    for (uint16_t j = 0; j < num_fruit; j++) {
        if (j == self) continue;
        if (fruit[j].x == x && fruit[j].y == y) return 0;
    }
    return 1;
}

/* If a corner is occupied by a stuck fruit, try to park "self" beside it. */
static uint8_t try_park_beside_corner(
    uint16_t self, uint8_t cx, uint8_t cy, uint8_t eo,
    Pixel* snake, uint16_t head, uint16_t len, uint16_t snake_max_size
) {
    uint8_t ax = cx, ay = cy;
    uint8_t bx = cx, by = cy;

    if (cx == 0 && cy == 0) {                       // top-left
        ax = 1; ay = 0;  bx = 0; by = 1;
    } else if (cx == (GAME_WIDTH-1) && cy == 0) {   // top-right
        ax = (uint8_t)(GAME_WIDTH-2); ay = 0;
        bx = (uint8_t)(GAME_WIDTH-1); by = 1;
    } else if (cx == 0 && cy == (GAME_HEIGHT-1)) {  // bottom-left
        ax = 1; ay = (uint8_t)(GAME_HEIGHT-1);
        bx = 0; by = (uint8_t)(GAME_HEIGHT-2);
    } else if (cx == (GAME_WIDTH-1) && cy == (GAME_HEIGHT-1)) { // bottom-right
        ax = (uint8_t)(GAME_WIDTH-2); ay = (uint8_t)(GAME_HEIGHT-1);
        bx = (uint8_t)(GAME_WIDTH-1); by = (uint8_t)(GAME_HEIGHT-2);
    } else {
        return 0;
    }

    uint8_t first_x  = eo ? bx : ax;
    uint8_t first_y  = eo ? by : ay;
    uint8_t second_x = eo ? ax : bx;
    uint8_t second_y = eo ? ay : by;

    if (can_place_fruit_here(self, first_x, first_y, snake, head, len, snake_max_size)) {
        vga_draw_pixel(fruit[self].x, fruit[self].y, VGA_COLOR_GREEN);
        fruit[self].x = first_x; fruit[self].y = first_y;
        vga_draw_pixel(first_x, first_y, VGA_COLOR_RED);
        return 1;
    }
    if (can_place_fruit_here(self, second_x, second_y, snake, head, len, snake_max_size)) {
        vga_draw_pixel(fruit[self].x, fruit[self].y, VGA_COLOR_GREEN);
        fruit[self].x = second_x; fruit[self].y = second_y;
        vga_draw_pixel(second_x, second_y, VGA_COLOR_RED);
        return 1;
    }
    return 0;
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

uint8_t fruit_is_at(uint8_t x, uint8_t y) {
    for (uint16_t i = 0; i < num_fruit; i++) {
        if (fruit[i].x == x && fruit[i].y == y) return 1;
    }
    return 0;
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


static uint8_t generate_valid_fruit_for_index(
    uint16_t index,Pixel* snake, uint16_t head, uint16_t len,
    uint16_t snake_max_size) {
    if (index >= FRUIT_MAX_NUM) return 0;

    /* guard to avoid infinite loop on crowded boards */
    const int MAX_ATTEMPTS = (GAME_HEIGHT * GAME_WIDTH);
    int attempts = 0;

    while (attempts++ < MAX_ATTEMPTS) {

        fruit_rng_stir(((uint32_t)attempts * 0xA341316Cu) ^
                       ((uint32_t)index << 24) ^
                       ((uint32_t)num_fruit << 16) ^
                       rng_rand());

        uint8_t rx = (uint8_t)(1 + (rng_rand() % (GAME_WIDTH  - 2)));
        uint8_t ry = (uint8_t)(1 + (rng_rand() % (GAME_HEIGHT - 2)));

        if (rx >= GAME_WIDTH || ry >= GAME_HEIGHT) continue;

        if (snake_collides(rx, ry, snake, head, len, snake_max_size)) continue;

        /* avoid overlapping other fruits */
        if (!can_place_fruit_here(index, rx, ry, snake, head, len, snake_max_size)) continue;

        fruit[index].x = rx;
        fruit[index].y = ry;
        return 1;
    }

    return 0;
}

static uint8_t try_move_fruit_1(
    uint16_t i, int8_t dx, int8_t dy, uint8_t eo,
    Pixel* snake, uint16_t head, uint16_t len, uint16_t snake_max_size
) {
    if (other_fruit[i]) return 0;
    uint8_t ox = fruit[i].x, oy = fruit[i].y;

    int16_t nx = (int16_t)ox + dx;
    int16_t ny = (int16_t)oy + dy;

    /* keep in-bounds (use your preferred border policy) */
    if (nx < 0) nx = 0;
    if (ny < 0) ny = 0;
    if (nx > (int16_t)(GAME_WIDTH  - 1)) nx = (int16_t)(GAME_WIDTH  - 1);
    if (ny > (int16_t)(GAME_HEIGHT - 1)) ny = (int16_t)(GAME_HEIGHT - 1);

    uint8_t ux = (uint8_t)nx, uy = (uint8_t)ny;

    if (ux == ox && uy == oy) return 0;
    if (snake_collides(ux, uy, snake, head, len, snake_max_size)) return 0;
    
    int16_t occ = fruit_index_at(ux, uy);
    if (occ >= 0 && (uint16_t)occ != i) {
        // If the occupied spot is a CORNER and that fruit is STUCK, park beside it
        if (other_fruit[occ] && is_corner(ux, uy)) {
            if (try_park_beside_corner(i, ux, uy, eo, snake, head, len, snake_max_size)) {
                return 1; // successfully parked next to corner fruit
            }
        }
        return 0; // otherwise blocked by another fruit
    }

    vga_draw_pixel(ox, oy, VGA_COLOR_GREEN);
    fruit[i].x = ux; fruit[i].y = uy;
    vga_draw_pixel(ux, uy, VGA_COLOR_RED);

    if (is_corner(ux, uy)) {
        other_fruit[i] = 1;
    }

    return 1;  
}

/* ---------- Eat + respawn ---------- */
uint8_t fruit_try_eat_and_respawn(
    uint8_t cur_x, uint8_t cur_y,
    uint8_t new_x, uint8_t new_y,
    Pixel*  snake, uint16_t head, uint16_t len, uint16_t snake_max_size,
    uint16_t* score, uint16_t* len_inout, Dir snake_dir, uint8_t respawn) {
    for (uint16_t i = 0; i < num_fruit; i++) {
        if (fruit[i].x == new_x && fruit[i].y == new_y) {
            /* eaten */
            if (score) (*score)++;
            if (len_inout && (*len_inout) < snake_max_size) (*len_inout)++;
            other_fruit[i] = 0;

            if (!generate_valid_fruit_for_index(i, snake, head, len, snake_max_size)) {
                fruit[i].x = (uint8_t)FRUIT_POSX;
                fruit[i].y = (uint8_t)FRUIT_POSY;
            }

            vga_draw_pixel(fruit[i].x, fruit[i].y, VGA_COLOR_RED);
            set_leds(0);
            return 1;
        }

        if (respawn && in_7x7_box(new_x, new_y, fruit[i].x, fruit[i].y) &&
            moving_toward(cur_x, cur_y, new_x, new_y, fruit[i].x, fruit[i].y)) {
            uint8_t eo = (score && ((*score) & 1)) ? 1 : 0;  /* 0=even, 1=odd */
            uint8_t moved = 0;

            /* Primary: push in snake direction */
            switch (snake_dir) {
                case DIR_RIGHT: moved = try_move_fruit_1(i,  1,  0, eo ,snake, head, len, snake_max_size); break;
                case DIR_LEFT:  moved = try_move_fruit_1(i, -1,  0, eo, snake, head, len, snake_max_size); break;
                case DIR_UP:    moved = try_move_fruit_1(i,  0, -1, eo, snake, head, len, snake_max_size); break;
                case DIR_DOWN:  moved = try_move_fruit_1(i,  0,  1, eo, snake, head, len, snake_max_size); break;
                default: break;
            }

            /* Secondary: if pinned, slide along the wall based on parity */
            if (!moved) {
                if (snake_dir == DIR_LEFT || snake_dir == DIR_RIGHT) {
                    /* on vertical wall: even->UP, odd->DOWN */
                    int8_t slide_dy = eo ? +1 : -1;
                    (void)try_move_fruit_1(i, 0, slide_dy, eo, snake, head, len, snake_max_size);
                } 
                else if (snake_dir == DIR_UP || snake_dir == DIR_DOWN) {
                    /* on horizontal wall: even->LEFT, odd->RIGHT */
                    int8_t slide_dx = eo ? +1 : -1;
                    (void)try_move_fruit_1(i, slide_dx, 0, eo, snake, head, len, snake_max_size);
                }
            }
        }
    }
    return 0;
}