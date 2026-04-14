#include "LCD_ILI9325.h" // edit based on code
#include "LCD_SDD1289.h" // edit based on code
#include "LPC17xx.h"
#include "Open1768_LCD.h"
#include "TP_Open1768.h"
#include "asciiLib.h"
#include "uart.h"
#include <stdint.h>

typedef uint8_t ASCIIchar[16];

inline uint32_t abs(int x) { return (x < 0) ? -x : x; }

typedef struct Point_t {
  uint16_t x;
  uint16_t y;
} Point;

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
  // oś wiodąca OX
  if (dx > dy) {
    ai = (dy - dx) * 2;
    bi = dy * 2;
    d = bi - dx;
    // pętla po kolejnych x
    while (x != x2) {
      // test współczynnika
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
  // oś wiodąca OY
  else {
    ai = (dx - dy) * 2;
    bi = dx * 2;
    d = bi - dy;
    // pętla po kolejnych y
    while (y != y2) {
      // test współczynnika
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
  /// TODO
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
  GetASCIICode(ASCII_8X16_System, ascii_char, chr);
  // SDD1289
  lcdWriteReg(HADRPOS_RAM, ((x + 8) << 8) + x);
  lcdWriteReg(VADRPOS_RAM_START, y);
  lcdWriteReg(VADRPOS_RAM_END, y + 16);

  // ILI9325
  lcdWriteReg(HARDPOS_RAM_START, x);
  lcdWriteReg(HARDPOS_RAM_END, (x + 8) << 7);
  lcdWriteReg(VADRPOS_RAM_START, y);
  lcdWriteReg(VADRPOS_RAM_END, y + 16);

  lcdWriteReg(ADRX_RAM, x);
  lcdWriteReg(ADRY_RAM, y);
  lcdWriteIndex(DATA_RAM);
  for (uint32_t i = 0; i < 16; i++) {
    for (uint32_t j = 0; j < 8; j++) {
      uint8_t mask = 1 << (7 - j);
      uint16_t bc = lcdReadData();
      lcdWriteData(mask & ascii_char[i] ? color : bc);
    }
  }
}

void draw_text(const char *str, uint16_t x, uint16_t y, uint16_t color) {
  while (*str) {
    draw_char(*str, x, y, color);
    str++;
    x += 8;
  }
}

uint32_t offset_y = 0, offset_x = 0;
float scale_y = 1, scale_x = 0;

void TP_get_mean_XY(uint32_t *x, uint32_t *y) {
  *x = 0;
  *y = 0;
  for (uint32_t i = 0; i < 20; i++) {
    int32_t tx = 0, ty = 0;
    touchpanelGetXY(&tx, &ty);
    *x += tx;
    *y -= ty;
  }
  *x /= 20;
  *y /= 20;
}

void TP_config() {
  uint32_t x1 = 10, y1 = 10;
  /// TODO draw cross (x1, y1)
  uint32_t tx1, ty1;
  TP_get_mean_XY(&tx1, &ty1);

  uint32_t x2 = LCD_MAX_X - 10, y2 = LCD_MAX_Y - 10;
  /// TODO draw cross (x1, y1)
  uint32_t tx2, ty2;
  TP_get_mean_XY(&tx2, &ty2);

  scale_x = (float)(x2 - x1) / (tx2 - tx1);
  scale_y = (float)(y2 - y1) / (ty2 - ty1);

  offset_x = x1 - scale_x * tx1;
  offset_y = y1 - scale_y * ty1;
}

void TP_to_LCD(uint32_t tp_x, uint32_t tp_y, uint32_t *lcd_x, uint32_t *lcd_y) {
  *lcd_x = tp_x * scale_x + offset_x;
  *lcd_y = tp_y * scale_y + offset_y;
}

int main() {
  UART_init_reg();
  lcdConfiguration();

  uint32_t lcd_controller_code = lcdReadReg(OSCIL_ON);
  char str[8];

  int_to_str(lcd_controller_code, str, 16);
  UART_write_string(str);

  // add lcd symbol, look readme
  //   init_SDD1289();
  //   init_ILI9325();

  //   for (uint32_t i = 0; i < 20; i++) {
  //     for (uint32_t j = 0; j < 20; j++) {
  //       draw_pixel(i, j, LCDGreen);
  //     }
  //   }

  touchpanelInit();

  for (;;) {
  }
}
