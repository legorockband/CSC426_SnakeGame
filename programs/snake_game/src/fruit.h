#ifndef FRUIT_H
#define FRUIT_H

#include <stdint.h>
#include "inputs.h"   /* for Pixel struct */

/* Configure fruit module (call once per game start) */
void fruit_set_count(uint8_t n);
uint8_t fruit_get_count(void);

/* Optional: switch->fruit-count helper (same mapping you had) */
uint8_t fruit_count_from_switches(uint16_t sw);

/* RNG seed used for random respawn placement */
void fruit_rng_seed(uint32_t seed);

/* Initialize fruit positions and draw them */
void fruit_init_and_draw(void);

/* Generate a random fruit position */
uint8_t fruit_rand(void);

/* Draw all current fruit pixels */
void fruit_draw_all(void);

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
    uint8_t new_x,
    uint8_t new_y,
    Pixel*  snake,
    uint16_t head,
    uint16_t len,
    uint16_t snake_max_size,
    uint16_t* score,
    uint16_t* len_inout
);

#endif