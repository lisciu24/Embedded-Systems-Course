#ifndef GENERATOR_CTRL_MOCK_H
#define GENERATOR_CTRL_MOCK_H

#include <stdint.h>
#include <stdio.h>

#define BITMAP_SIZE 8

static inline void GENCTRL_init(void) {}

static inline void GENCTRL_function(const uint16_t fun[], uint32_t amplitude,
                                    uint32_t frequency) {
    (void)fun;
    printf("GENCTRL_function(fun, %d, %d)\n", amplitude, frequency);
}

static inline void GENCTRL_stop(void) { printf("GENCTRL_stop()\n"); }

static inline void GENCTRL_start(void) { printf("GENCTRL_start()\n"); }

static inline void GENCTRL_bitmap(void) { printf("GENCTRL_bitmap()\n"); }

static inline void GENCTRL_load_bitmap_row(uint32_t bmp_row,
                                           uint32_t bmp_row_idx) {
    printf("GENCTRL_load_bitmap_row(%d, %d)\n", bmp_row, bmp_row_idx);
}

static inline void GENCTRL_bitmap_row(uint32_t bmp_buff_idx,
                                      uint32_t bmp_row_idx) {
    (void)bmp_buff_idx;
    (void)bmp_row_idx;
    printf("GENCTRL_bitmap_row(%d, %d)\n", bmp_buff_idx, bmp_row_idx);
}

#endif // GENERATOR_CTRL_MOCK_H