#ifndef LCD_H
#define LCD_H

#include "LPC17xx.h"
#include "lcd_lib/LCD_ILI9325.h"
#include "lcd_lib/Open1768_LCD.h"
#include "tp_lib/TP_Open1768.h"

typedef struct Point_t 
{
	uint16_t x;
	uint16_t y;
} Point;

uint16_t fix_color(uint16_t color);
uint16_t read_pixel_color(uint16_t x, uint16_t y);
void draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void reset_window(void);
void fill_screen_brute(uint16_t color);
void fill_screen_fast(uint16_t color);
void draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void draw_poly(Point *points, uint32_t n, uint16_t color);
void TP_get_mean_XY(volatile uint32_t *x, volatile uint32_t *y);
void TP_to_LCD(uint32_t tp_x, uint32_t tp_y, uint32_t *lcd_x, uint32_t *lcd_y);
void TP_config(void);

#endif //LCD_H
