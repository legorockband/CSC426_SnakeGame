#ifndef DRAW_H
#define DRAW_H

#include <stdint.h>

// Hardware-visible framebuffer window is 80x60
#define GAME_WIDTH   80
#define GAME_HEIGHT  60

void draw_block(uint8_t x, uint8_t y, uint8_t color);
void draw_char_5x7(uint8_t x, uint8_t y, char c, uint8_t color);
void draw_text_5x7(uint8_t x, uint8_t y, const char *s, uint8_t color);
void draw_text_centered_5x7(uint8_t y, const char *s, uint8_t color);
void draw_uint_5x7(uint8_t x, uint8_t y, uint16_t value, uint8_t color);
void draw_label_value_centered_5x7(uint8_t y, const char *label, uint16_t value, uint8_t color);

void draw_home_screen(uint8_t start_selected, uint16_t score, uint16_t high_score);
void draw_exit_screen(uint16_t high_score);
void draw_win_screen(uint16_t score, uint16_t high_score);

#endif