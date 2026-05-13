#ifndef CALLIGRAPHY_H
#define CALLIGRAPHY_H

#include "LPC17xx.h"
#include "lcd_lib/LCD_ILI9325.h"
#include "lcd_lib/Open1768_LCD.h"
#include "lcd_lib/asciiLib.h"

void draw_char(uint8_t chr, uint16_t x, uint16_t y, uint16_t color, uint16_t bg_color);
void draw_text(const char *str, uint16_t x, uint16_t y, uint16_t color, uint16_t bg_color);
void draw_char_vertical(uint8_t chr, uint16_t x, uint16_t y, uint16_t color, uint16_t bg_color);
void draw_text_vertical(const char *str, uint16_t x, uint16_t y, uint16_t color, uint16_t bg_color);

#endif //CALLIGRAPHY_H