#ifndef MMIO_H
#define MMIO_H

#include <stdint.h>

#define MMIO32(addr) (*(volatile uint32_t*)(addr))
#endif
