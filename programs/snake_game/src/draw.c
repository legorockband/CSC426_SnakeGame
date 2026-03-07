#include <stdint.h>
#include "vga.h"
#include "outputs.h"
#include "draw.h"

static uint8_t text_width_5x7(const char *s) {
    uint8_t width = 0;
    while (*s) {
        width += 6;   // 5 pixels wide + 1 space
        s++;
    }
    if (width > 0) width -= 1; // no trailing space after last char
    return width;
}

static uint8_t uint_width_5x7(uint16_t value) {
    uint8_t digits = 1;
    while (value >= 10) {
        value /= 10;
        digits++;
    }
    return (uint8_t)(digits * 6 - 1);
}

void draw_block(uint8_t x, uint8_t y, uint8_t color) {
    vga_draw_pixel(x, y, color);
}

void draw_char_5x7(uint8_t x, uint8_t y, char c, uint8_t color) {
    const uint8_t *glyph = 0;

    static const uint8_t A[7] = {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11};
    static const uint8_t B[7] = {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E};
    static const uint8_t C[7] = {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E};
    static const uint8_t E[7] = {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F};
    static const uint8_t G[7] = {0x0E,0x11,0x10,0x17,0x11,0x11,0x0E};
    static const uint8_t H[7] = {0x11,0x11,0x11,0x1F,0x11,0x11,0x11};
    static const uint8_t I[7] = {0x1F,0x04,0x04,0x04,0x04,0x04,0x1F};
    static const uint8_t K[7] = {0x11,0x12,0x14,0x18,0x14,0x12,0x11};
    static const uint8_t L[7] = {0x10,0x10,0x10,0x10,0x10,0x10,0x1F};
    static const uint8_t M[7] = {0x11,0x1B,0x15,0x15,0x11,0x11,0x11};
    static const uint8_t N[7] = {0x11,0x19,0x15,0x13,0x11,0x11,0x11};
    static const uint8_t O[7] = {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E};
    static const uint8_t P[7] = {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10};
    static const uint8_t R[7] = {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11};
    static const uint8_t S[7] = {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E};
    static const uint8_t T[7] = {0x1F,0x04,0x04,0x04,0x04,0x04,0x04};
    static const uint8_t U[7] = {0x11,0x11,0x11,0x11,0x11,0x11,0x0E};
    static const uint8_t V[7] = {0x11,0x11,0x11,0x11,0x11,0x0A,0x04};
    static const uint8_t W[7] = {0x11,0x11,0x11,0x15,0x15,0x1B,0x11};
    static const uint8_t X[7] = {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11};

    static const uint8_t N0[7] = {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E};
    static const uint8_t N1[7] = {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E};
    static const uint8_t N2[7] = {0x0E,0x11,0x01,0x02,0x04,0x08,0x1F};
    static const uint8_t N3[7] = {0x1E,0x01,0x01,0x0E,0x01,0x01,0x1E};
    static const uint8_t N4[7] = {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02};
    static const uint8_t N5[7] = {0x1F,0x10,0x10,0x1E,0x01,0x01,0x1E};
    static const uint8_t N6[7] = {0x0E,0x10,0x10,0x1E,0x11,0x11,0x0E};
    static const uint8_t N7[7] = {0x1F,0x01,0x02,0x04,0x08,0x08,0x08};
    static const uint8_t N8[7] = {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E};
    static const uint8_t N9[7] = {0x0E,0x11,0x11,0x0F,0x01,0x01,0x0E};

    switch (c) {
        case 'A': glyph = A; break;
        case 'B': glyph = B; break;
        case 'C': glyph = C; break;
        case 'E': glyph = E; break;
        case 'G': glyph = G; break;
        case 'H': glyph = H; break;
        case 'I': glyph = I; break;
        case 'K': glyph = K; break;
        case 'L': glyph = L; break;
        case 'M': glyph = M; break;
        case 'N': glyph = N; break;
        case 'O': glyph = O; break;
        case 'P': glyph = P; break;
        case 'R': glyph = R; break;
        case 'S': glyph = S; break;
        case 'T': glyph = T; break;
        case 'U': glyph = U; break;
        case 'V': glyph = V; break;
        case 'W': glyph = W; break;
        case 'X': glyph = X; break;

        case '0': glyph = N0; break;
        case '1': glyph = N1; break;
        case '2': glyph = N2; break;
        case '3': glyph = N3; break;
        case '4': glyph = N4; break;
        case '5': glyph = N5; break;
        case '6': glyph = N6; break;
        case '7': glyph = N7; break;
        case '8': glyph = N8; break;
        case '9': glyph = N9; break;

        case ' ': glyph = 0; break;
        default:  glyph = 0; break;
    }

    if (!glyph) return;

    for (uint8_t row = 0; row < 7; row++) {
        for (uint8_t col = 0; col < 5; col++) {
            if (glyph[row] & (1 << (4 - col))) {
                draw_block(x + col, y + row, color);
            }
        }
    }
}

void draw_text_5x7(uint8_t x, uint8_t y, const char *s, uint8_t color) {
    while (*s) {
        draw_char_5x7(x, y, *s, color);
        x += 6;
        s++;
    }
}

void draw_text_centered_5x7(uint8_t y, const char *s, uint8_t color) {
    uint8_t w = text_width_5x7(s);
    uint8_t x = (GAME_WIDTH - w) / 2;
    draw_text_5x7(x, y, s, color);
}

void draw_uint_5x7(uint8_t x, uint8_t y, uint16_t value, uint8_t color) {
    char buf[6];
    uint8_t i = 0;
    uint8_t j;

    if (value == 0) {
        draw_char_5x7(x, y, '0', color);
        return;
    }

    while (value > 0 && i < sizeof(buf)) {
        buf[i++] = (char)('0' + (value % 10));
        value /= 10;
    }

    for (j = 0; j < i; j++) {
        draw_char_5x7(x + (uint8_t)(j * 6), y, buf[i - 1 - j], color);
    }
}

void draw_label_value_centered_5x7(uint8_t y, const char *label, uint16_t value, uint8_t color) {
    uint8_t label_w = text_width_5x7(label);
    uint8_t value_w = uint_width_5x7(value);
    uint8_t total_w = label_w + 6 + value_w;   // one space between label and number
    uint8_t x = (GAME_WIDTH - total_w) / 2;

    draw_text_5x7(x, y, label, color);
    draw_uint_5x7((uint8_t)(x + label_w + 6), y, value, color);
}

void draw_home_screen(uint8_t start_selected, uint16_t score, uint16_t high_score) {
    vga_fill(VGA_COLOR_GREEN);

    draw_text_centered_5x7(4, "SNAKE", VGA_COLOR_BLUE);
    draw_label_value_centered_5x7(14, "SCORE", score, VGA_COLOR_RED);
    draw_label_value_centered_5x7(22, "HIGH", high_score, VGA_COLOR_BLUE);

    if (start_selected) {
        draw_text_centered_5x7(36, "START", VGA_COLOR_RED);
        draw_text_centered_5x7(46, "EXIT", VGA_COLOR_BLUE);
    } else {
        draw_text_centered_5x7(36, "START", VGA_COLOR_BLUE);
        draw_text_centered_5x7(46, "EXIT", VGA_COLOR_RED);
    }

    set_sseg(high_score);
}

void draw_exit_screen(uint16_t high_score) {
    vga_fill(VGA_COLOR_RED);

    draw_text_centered_5x7(10, "EXIT", VGA_COLOR_BLUE);
    draw_label_value_centered_5x7(24, "HIGH", high_score, VGA_COLOR_GREEN);
    draw_text_centered_5x7(40, "MENU", VGA_COLOR_GREEN);

    set_sseg(high_score);
}

void draw_win_screen(uint16_t score, uint16_t high_score) {
    vga_fill(VGA_COLOR_BLUE);

    draw_text_centered_5x7(6, "GAME OVER", VGA_COLOR_RED);
    draw_label_value_centered_5x7(20, "SCORE", score, VGA_COLOR_GREEN);
    draw_label_value_centered_5x7(30, "HIGH", high_score, VGA_COLOR_GREEN);
    draw_text_centered_5x7(44, "MENU", VGA_COLOR_RED);

    set_sseg(score);
}