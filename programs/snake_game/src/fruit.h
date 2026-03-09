#ifndef FRUIT_H
#define FRUIT_H

#include <stdint.h>
#include "inputs.h"   /* for Pixel struct */

#define FRUIT_POSX 60
#define FRUIT_POSY 30
#define FRUIT_MAX_NUM  5   

typedef enum { DIR_NONE, DIR_RIGHT, DIR_LEFT, DIR_UP, DIR_DOWN } Dir;

/* Configure fruit module (call once per game start) */
void fruit_set_count(uint8_t n);
uint8_t fruit_get_count(void);

/* Optional: switch->fruit-count helper (same mapping you had) */
uint8_t fruit_count_from_switches(uint16_t sw);

/* RNG seed used for random respawn placement */
void fruit_rng_seed(uint32_t seed);

void fruit_rng_stir(uint32_t v);

void fruit_spawn_entropy(uint32_t v);

/* Initialize fruit positions and draw them */
void fruit_init_and_draw(void);

/* Generate a random fruit position */
uint8_t fruit_rand(void);

/* Draw all current fruit pixels */
void fruit_draw_all(void);

uint8_t fruit_is_at(uint8_t x, uint8_t y);

/*
 * Check if snake is moving onto a fruit.
 * If yes: increments *score and *len (if < snake_max_size),
 *         respawns that fruit to a valid random spot,
 *         draws the respawned fruit,
 *         returns 1.
 * If no: returns 0.
 *
 * Needs snake info to avoid respawning on the snake body.
 */
uint8_t fruit_try_eat_and_respawn(
    uint8_t cur_x, uint8_t cur_y,
    uint8_t new_x, uint8_t new_y,
    Pixel*  snake, uint16_t head, uint16_t len, uint16_t snake_max_size,
    uint16_t* score, uint16_t* len_inout, Dir snake_dir, uint8_t respawn
);

#endif