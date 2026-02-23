#include "inputs.h"

uint16_t read_switches(void) {
    return (uint16_t)(SW_IN & 0xFFFF);
}

uint8_t read_btn_l(void) {
    return (uint8_t)(BTN_L & 1);
}

uint8_t read_btn_r(void) {
    return (uint8_t)(BTN_R & 1);
}

uint8_t read_btn_u(void) {
    return (uint8_t)(BTN_U & 1);
}

uint8_t read_btn_d(void) {
    return (uint8_t)(BTN_D & 1);
}

uint8_t read_buttons(void) {
    uint8_t btnr = read_btn_r();
    uint8_t btnl = read_btn_l();
    uint8_t btnu = read_btn_u();
    uint8_t btnd = read_btn_d();

    return (btnr << 3) | (btnl << 2) | (btnu << 1) | btnd;
}