#ifndef LCD_H
#define LCD_H

#include "LPC17xx.h"
#include "lcd_lib/LCD_ILI9325.h"
#include "lcd_lib/Open1768_LCD.h"
#include "lcd_lib/asciiLib.h"
#include "tp_lib/TP_Open1768.h"
#include "systick.h"

typedef struct Point_t {
  uint16_t x;
  uint16_t y;
} Point;

uint16_t fix_color(uint16_t color) {
  uint16_t fixed = 0;
	for(uint32_t i = 0; i < 8; i++) {
    uint16_t lsb = color & (1 << i);
    uint16_t msb = color & (1 << (15 - i));
    fixed |= (lsb << (15 - 2 * i)) | (msb >> (15 - 2 * i));
	}
	return fixed;
}

uint16_t read_pixel_color(uint16_t x, uint16_t y)
{
    // setting coordinates
    lcdWriteReg(ADRX_RAM, x);
    lcdWriteReg(ADRY_RAM, y);

    // selecting data register
    lcdWriteIndex(DATA_RAM);

    return fix_color(lcdReadData());
}

void reset_window() {
  lcdWriteReg(HADRPOS_RAM_START, 0);
  lcdWriteReg(HADRPOS_RAM_END, LCD_MAX_X);
  lcdWriteReg(VADRPOS_RAM_START, 0);
  lcdWriteReg(VADRPOS_RAM_END, LCD_MAX_Y);
}

void draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
  lcdWriteReg(ADRX_RAM, x);
  lcdWriteReg(ADRY_RAM, y);
  lcdWriteReg(DATA_RAM, color);
}

void fill_screen_brute(uint16_t color) {
  for (uint16_t x = 0; x < LCD_MAX_X; x++) {
    for (uint16_t y = 0; y < LCD_MAX_Y; y++) {
      draw_pixel(x, y, color);
    }
  }
}

void fill_screen_fast(uint16_t color) {
  lcdWriteReg(ADRX_RAM, 0);
  lcdWriteReg(ADRY_RAM, 0);
  lcdWriteIndex(DATA_RAM);
  for (uint32_t i = 0; i < LCD_MAX_X * LCD_MAX_Y; i++) {
    lcdWriteData(color);
  }
}

void draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2,
               uint16_t color) {
  // zmienne pomocnicze
  int d, dx, dy, ai, bi, xi, yi;
  int x = x1, y = y1;
  // ustalenie kierunku rysowania
  if (x1 < x2) {
    xi = 1;
    dx = x2 - x1;
  } else {
    xi = -1;
    dx = x1 - x2;
  }
  // ustalenie kierunku rysowania
  if (y1 < y2) {
    yi = 1;
    dy = y2 - y1;
  } else {
    yi = -1;
    dy = y1 - y2;
  }
  // pierwszy piksel
  draw_pixel(x, y, color);
  // os wiodaca OX
  if (dx > dy) {
    ai = (dy - dx) * 2;
    bi = dy * 2;
    d = bi - dx;
    // petla po kolejnych x
    while (x != x2) {
      // test wspólczynnika
      if (d >= 0) {
        x += xi;
        y += yi;
        d += ai;
      } else {
        d += bi;
        x += xi;
      }
      draw_pixel(x, y, color);
    }
  }
  // os wiodaca OY
  else {
    ai = (dx - dy) * 2;
    bi = dx * 2;
    d = bi - dy;
    // petla po kolejnych y
    while (y != y2) {
      // test wspólczynnika
      if (d >= 0) {
        x += xi;
        y += yi;
        d += ai;
      } else {
        d += bi;
        y += yi;
      }
      draw_pixel(x, y, color);
    }
  }
}

void draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
  draw_line(x, y, x + w, y, color);
  draw_line(x + w, y, x + w, y + h, color);
  draw_line(x + w, y + h, x, y + h, color);
  draw_line(x, y + h, x, y, color);
}

void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
  lcdWriteReg(HADRPOS_RAM_START, x);
  lcdWriteReg(HADRPOS_RAM_END, x + w);
  lcdWriteReg(VADRPOS_RAM_START, y);
  lcdWriteReg(VADRPOS_RAM_END, y + h);
	
  lcdWriteReg(ADRX_RAM, x);
  lcdWriteReg(ADRY_RAM, y);
  lcdWriteIndex(DATA_RAM);
  for (uint32_t i = 0; i < w * h; i++) {
    lcdWriteData(color);
  }
  reset_window();
}

void draw_poly(Point *points, uint32_t n, uint16_t color) {
  for (uint32_t i = 0; i < n - 1; i++) {
    draw_line(points[i].x, points[i].y, points[i + 1].x, points[i + 1].y,
              color);
  }
}

void draw_char(uint8_t chr, uint16_t x, uint16_t y, uint16_t color) {
  // char is 8x16 (w x h)
  // top to bottom line by line
  uint8_t ascii_char[16];
  GetASCIICode(1, ascii_char, chr);

  lcdWriteReg(HADRPOS_RAM_START, x);
  lcdWriteReg(HADRPOS_RAM_END, x + 7);
  lcdWriteReg(VADRPOS_RAM_START, y);
  lcdWriteReg(VADRPOS_RAM_END, y + 15);

  lcdWriteReg(ADRX_RAM, x);
  lcdWriteReg(ADRY_RAM, y);
  lcdWriteIndex(DATA_RAM);
  for (uint32_t i = 0; i < 16; i++) {
    for (uint32_t j = 0; j < 8; j++) { 
      uint8_t mask = 1 << (7 - j);
		//fix_color(lcdReadData())
      lcdWriteData(mask & ascii_char[i] ? color : LCDWhite);
    }
  }
  
  reset_window();
}

void draw_text(const char *str, uint16_t x, uint16_t y, uint16_t color) {
  while (*str) {
    draw_char(*str, x, y, color);
    str++;
    x += 8;
  }
}

volatile int32_t offset_y = 0, offset_x = 0;
volatile int32_t scale_y = 850, scale_x = 420;

void TP_get_mean_XY(volatile uint32_t *x, volatile uint32_t *y) {
	uint32_t samples = 50;
  *x = 0;
  *y = 0;
  for (uint32_t i = 0; i < samples; i++) {
    int32_t tx = 0, ty = 0;
    touchpanelGetXY(&tx, &ty);
    *x += tx;
    *y += ty;
  }
  *x /= samples;
  *y /= samples;
}

void TP_config() {
  fill_screen_fast(LCDBlack);
  uint32_t x1 = 40, y1 = 40;
  draw_line(x1 - 10, y1 - 10, x1 + 10, y1 + 10, LCDMagenta);
  draw_line(x1 - 10, y1 + 10, x1 + 10, y1 - 10, LCDMagenta);
  uint32_t tx1, ty1;
	// wait for tp irq
  while (LPC_GPIO0->FIOPIN & (1 << 19))
		;
  TP_get_mean_XY(&tx1, &ty1);

  fill_screen_fast(LCDBlueSea);
  SYSTICK_wait(1000);
  fill_screen_fast(LCDBlack);
  uint32_t x2 = LCD_MAX_X - x1, y2 = LCD_MAX_Y - y1;
  draw_line(x2 - 10, y2 - 10, x2 + 10, y2 + 10, LCDMagenta);
  draw_line(x2 - 10, y2 + 10, x2 + 10, y2 - 10, LCDMagenta);  
  uint32_t tx2, ty2;
		// wait for tp irq
  while (LPC_GPIO0->FIOPIN & (1 << 19))
		;
  TP_get_mean_XY(&tx2, &ty2);
  

  scale_x = (x2 - x1) * 10000 / (ty2 - ty1);
  scale_y = (y2 - y1) * 10000 / (tx2 - tx1);

  offset_x = x1 - scale_x * ty1 / 10000;
  offset_y = y1 - scale_y * tx1 / 10000;
}

void TP_to_LCD(uint32_t tp_x, uint32_t tp_y, uint32_t *lcd_x, uint32_t *lcd_y) {
  *lcd_x = tp_y * scale_x / 10000 + offset_x;
  *lcd_y = tp_x * scale_y / 10000 + offset_y;
}

#endif //LCD_H
