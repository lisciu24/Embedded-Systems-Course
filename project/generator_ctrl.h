#ifndef GENERATOR_CTRL_H
#define GENERATOR_CTRL_H

#include "LPC17xx.h"
#include <stdint.h>

// MUST be above 40
#define FSAMPLE 50
#define MAX_AMPLITUDE 3300
#define MAX_FREQUENCY (25000000 / 25 / FSAMPLE)
#define MIN_FREQUENCY 10
#define BITMAP_SIZE 8
#define PIXEL_RES 8

void GENCTRL_init(void);
void GENCTRL_function(const uint16_t fun[], uint32_t amplitude,
                      uint32_t frequency);
void GENCTRL_stop(void);
void GENCTRL_start(void);
void GENCTRL_bitmap();
void GENCTRL_load_bitmap_row(uint32_t bmp_row, uint32_t bmp_row_idx);
void GENCTRL_bitmap_row(uint32_t, uint32_t);

#endif // GENERATOR_CTRL_H
