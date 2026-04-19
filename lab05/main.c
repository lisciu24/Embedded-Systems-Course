#include "lcd_lib/LCD_ILI9325.h"
#include "LPC17xx.h"
#include "lcd_lib/Open1768_LCD.h"
#include "tp_lib/TP_Open1768.h"
#include "lcd_lib/asciiLib.h"
#include "uart.h"
#include <stdint.h>
#include <stdio.h>

volatile uint32_t ticks = 0; 
void SysTick_Handler(void) {
	ticks++;
}

void wait(uint32_t _ticks) {
	uint32_t stop = ticks + _ticks;
	while(ticks != stop) 
		continue;
}

typedef uint8_t ASCIIchar[16];

inline uint32_t abs(int x) { return (x < 0) ? -x : x; }

typedef struct Point_t {
  uint16_t x;
  uint16_t y;
} Point;

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
      uint16_t bc = LCDBlack;
      lcdWriteData(mask & ascii_char[i] ? color : bc);
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
volatile float scale_y = 1, scale_x = 1;

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
  UART_write_string("read touch\n\r");
  TP_get_mean_XY(&tx1, &ty1);
  char buff[128];
	  sprintf(buff, "tx: %d ty: %d lx: %d ly: %d\n\r", tx1, ty1, x1, y1);
	UART_write_string(buff);
  
  wait(1000);
  
  fill_screen_fast(LCDBlack);
  uint32_t x2 = LCD_MAX_X - x1, y2 = LCD_MAX_Y - y1;
  draw_line(x2 - 10, y2 - 10, x2 + 10, y2 + 10, LCDMagenta);
  draw_line(x2 - 10, y2 + 10, x2 + 10, y2 - 10, LCDMagenta);  
  uint32_t tx2, ty2;
		// wait for tp irq
  while (LPC_GPIO0->FIOPIN & (1 << 19))
		;
  UART_write_string("read touch\n\r");
  TP_get_mean_XY(&tx2, &ty2);
  
  sprintf(buff, "tx: %d ty: %d lx: %d ly: %d\n\r", tx2, ty2, x2, y2);
	UART_write_string(buff);

  scale_x = (float)(x2 - x1) / (ty2 - ty1);
  scale_y = (float)(y2 - y1) / (tx2 - tx1);

  offset_x = x1 - scale_x * ty1;
  offset_y = y1 - scale_y * tx1;
  
  sprintf(buff, "sx: %d sy: %d ox: %d oy: %d\n\r", (int)scale_x, (int)scale_y, offset_x, offset_y);
	UART_write_string(buff);
}

void TP_to_LCD(uint32_t tp_x, uint32_t tp_y, uint32_t *lcd_x, uint32_t *lcd_y) {
  *lcd_x = tp_y * scale_x + offset_x;
  *lcd_y = tp_x * scale_y + offset_y;
}

void EINT0_init(void) {
  //led setup
  LPC_GPIO0->FIODIR |= 0x01;
  LPC_GPIO0->FIOPIN |= 0x01;
  // EINT setup
  LPC_SC->EXTINT = 0x01;                    // Clear EINT0 interrupt flag
  LPC_SC->EXTMODE |= 0x01;                  // Set EINT0 to edge-sensitive
  LPC_SC->EXTPOLAR &= ~0x01;                // Set EINT0 to rising edge
  LPC_SC->EXTINT = 0x01;                    // Clear EINT0 interrupt flag
  LPC_PINCON->PINSEL4 &= ~(0x03 << 20);     // Clear P2.10 function
  LPC_PINCON->PINSEL4 |= (0x01 << 20);      // Set P2.10 to EINT0
  LPC_PINCON->PINMODE4 &= ~(0x03 << 20);    // Set P2.10 to pull-up mode
  LPC_PINCON->PINMODE_OD2 &= ~(0x01 << 10); // Set P2.10 to normal mode
  LPC_SC->EXTINT = 0x01;                    // Clear EINT0 interrupt flag
  NVIC_EnableIRQ(EINT0_IRQn);               // Enable EINT0 interrupt in NVIC
}

volatile uint32_t flag = 0;

void EINT0_IRQHandler(void) {
  LPC_SC->EXTINT = 0x01; // Clear EINT0 interrupt flag
	flag ^= 1;
	fill_screen_fast(flag ? LCDGinger : LCDCyan);
	UART_write_string("Click!\r\n");
}

int main() {
  SystemInit();
  SysTick_Config(SystemCoreClock / 1000);
	
  UART_init_reg();
  lcdConfiguration();
  UART_write_string("UART LPC1768\r\n");

  uint32_t lcd_controller_code = lcdReadReg(OSCIL_ON);
  char str[8];

  int_to_str(lcd_controller_code, str, 16);
  UART_write_string(str);

  init_ILI9325();
  EINT0_init();

  // draw block
  for (uint32_t i = 0; i < 40; i++) {
    for (uint32_t j = 0; j < 20; j++) {
      draw_pixel(i, j, LCDGreen);
    }
  }
  
  // fill screen
  fill_screen_fast(LCDWhite);

  // draw line
  draw_line(10, 20, 100, 200, LCDBlack);
  draw_line(100, 150, 10, 60, LCDBlack);
  
  draw_rect(100, 100, 50, 50, LCDBlue);
  fill_rect(150, 120, 50, 50, LCDRed);
  
  draw_char('A', 100, 10, LCDMagenta);
  draw_text("LPC 1768", 100, 30, LCDBlueSea);
  
  touchpanelInit();
  
  TP_config();
  fill_screen_fast(LCDWhite);
  
  for (;;) {
	uint32_t tx = 0, ty = 0;
	while (LPC_GPIO0->FIOPIN & (1 << 19))
		;
	TP_get_mean_XY(&tx, &ty);
	  
	uint32_t lx = 0, ly = 0;
	TP_to_LCD(tx, ty, &lx, &ly);
	  
	draw_pixel(lx, ly, LCDBlack);
	 char buff[64];
	  sprintf(buff, "tx: %d ty: %d lx: %d ly: %d\n\r", tx, ty, lx, ly);
	UART_write_string(buff);
  }
}
