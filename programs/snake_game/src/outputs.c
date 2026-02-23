#include "outputs.h"

void set_leds(uint16_t value) {
    LED_OUT = value;
}

void set_sseg(uint16_t value) {
    SSEG_OUT = value;
}

