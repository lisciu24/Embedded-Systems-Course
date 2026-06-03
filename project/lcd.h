#ifndef LCD_H
#define LCD_H

#include "LPC17xx.h"
#include "lcd_lib/LCD_ILI9325.h"
#include "lcd_lib/Open1768_LCD.h"
#include "tp_lib/TP_Open1768.h"
#include <stdint.h>

typedef struct Point_t {
    uint16_t x;
    uint16_t y;
} Point;

typedef struct Calibration_Matrix_t {
    int16_t A, B, C;
    int16_t D, E, F;
} Calibration_Matrix;

uint16_t fix_color(uint16_t color);
uint16_t read_pixel_color(uint16_t x, uint16_t y);
void draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void reset_window(void);
void fill_screen_brute(uint16_t color);
void fill_screen_fast(uint16_t color);
void draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
               uint16_t color);
void draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void draw_poly(Point *points, uint32_t n, uint16_t color);
Point TP_get_mean_XY(void);
Point TP_to_LCD(const Point tp);
void TP_config_restore(void);
void TP_config_store(void);
void TP_config(void);

#endif // LCD_H
