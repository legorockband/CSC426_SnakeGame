#ifndef OUTPUTS_H
#define OUTPUTS_H

#include <stdint.h>

#define MMIO32(addr) (*(volatile uint32_t*)(addr))

#define LED_OUT    MMIO32(0x11000020u)
#define SSEG_OUT   MMIO32(0x11000040u)

void set_leds(uint16_t value);
void set_sseg(uint16_t value);

#endif