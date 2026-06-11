#ifndef GENERATOR_CTRL_H
#define GENERATOR_CTRL_H

#include "LPC17xx.h"
#include <stdint.h>

// MUST be above 40
#define FSAMPLE 50
#define MAX_AMPLITUDE 3300
#define MAX_FREQUENCY (25000000 / 25 / FSAMPLE)
#define MIN_FREQUENCY 10

void GENCTRL_init(void);
void GENCTRL_function(const uint16_t fun[], uint32_t amplitude,
                      uint32_t frequency);
void GENCTRL_stop(void);
void GENCTRL_start(void);
void GENCTRL_bitmap();
void GENCTRL_bitmap_row(uint32_t bmp_idx, uint32_t bmp_row_idx);

#endif // GENERATOR_CTRL_H
