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