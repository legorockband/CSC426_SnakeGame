#ifndef INPUTS_H
#define INPUTS_H

#include <stdint.h>
#include "mmio.h"

// Switches (lower 16 bits used)
#define SW_IN      MMIO32(0x11000000u)

// To add other switchs just add 1 to the hex value 
#define SW0_MASK 0x0001u    

// Buttons (bit 0 used)
#define BTN_L      MMIO32(0x11000220u)
#define BTN_R      MMIO32(0x11000240u)
#define BTN_U      MMIO32(0x11000260u)
#define BTN_D      MMIO32(0x11000280u)

#define BTN_R_MASK 0x08u
#define BTN_L_MASK 0x04u
#define BTN_U_MASK 0x02u
#define BTN_D_MASK 0x01u

// Optional helper function prototypes
uint16_t read_switches(void);
uint8_t  read_btn_l(void);
uint8_t  read_btn_r(void);
uint8_t  read_btn_u(void);
uint8_t  read_btn_d(void);
uint8_t read_buttons(void);

#endif