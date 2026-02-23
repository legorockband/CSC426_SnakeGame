#ifndef INPUTS_H
#define INPUTS_H

#include <stdint.h>

#define MMIO32(addr) (*(volatile uint32_t*)(addr))

// Switches (lower 16 bits used)
#define SW_IN      MMIO32(0x11000000u)

// Buttons (bit 0 used)
#define BTN_L      MMIO32(0x11000220u)
#define BTN_R      MMIO32(0x11000240u)
#define BTN_U      MMIO32(0x11000260u)
#define BTN_D      MMIO32(0x11000280u)

// Optional helper function prototypes
uint16_t read_switches(void);
uint8_t  read_btn_l(void);
uint8_t  read_btn_r(void);
uint8_t  read_btn_u(void);
uint8_t  read_btn_d(void);

#endif