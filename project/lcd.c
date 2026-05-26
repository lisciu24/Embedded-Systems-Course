#include "lcd.h"
#include "systick.h"
#include "uart.h"

uint16_t fix_color(uint16_t color)
{
    uint16_t fixed = 0;
    for(uint32_t i = 0; i < 8; i++)
    {
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

    //lcdReadData(); // dummy read 9. GRAM Address Map & Read/Write on page 79 of ILI9325 Version: 0.43

    return fix_color(lcdReadData());
}

void draw_pixel(uint16_t x, uint16_t y, uint16_t color) 
{
	lcdWriteReg(ADRX_RAM, x);
	lcdWriteReg(ADRY_RAM, y);
	lcdWriteReg(DATA_RAM, color);
}

void reset_window()
{
    lcdWriteReg(HADRPOS_RAM_START, 0);
    lcdWriteReg(HADRPOS_RAM_END, LCD_MAX_X);
    lcdWriteReg(VADRPOS_RAM_START, 0);
    lcdWriteReg(VADRPOS_RAM_END, LCD_MAX_Y);
}

void fill_screen_brute(uint16_t color)
{
    for (uint16_t x = 0; x < LCD_MAX_X; x++)
    {
        for (uint16_t y = 0; y < LCD_MAX_Y; y++)
        {
            draw_pixel(x, y, color);
        }
    }
}

void fill_screen_fast(uint16_t color)
{
    lcdWriteReg(ADRX_RAM, 0);
    lcdWriteReg(ADRY_RAM, 0);
    lcdWriteIndex(DATA_RAM);
    for (uint32_t i = 0; i < LCD_MAX_X * LCD_MAX_Y; i++)
    {
        lcdWriteData(color);
    }
}

//
void draw_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
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
      // test wsp�lczynnika
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
      // test wsp�lczynnika
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

// all values are inclusive
void draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    draw_line(x, y, x + w + 1, y, color); // horizontal left
    draw_line(x + w + 1, y, x + w + 1, y + h + 1, color); // vertical top
    draw_line(x + w + 1, y + h + 1, x, y + h + 1, color); // horizontal right
    draw_line(x, y + h + 1, x, y, color); // vertical bottom
}

// all values are inclusive
void fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    lcdWriteReg(HADRPOS_RAM_START, x);
    lcdWriteReg(HADRPOS_RAM_END, x + w);
    lcdWriteReg(VADRPOS_RAM_START, y);
    lcdWriteReg(VADRPOS_RAM_END, y + h);
        
    lcdWriteReg(ADRX_RAM, x);
    lcdWriteReg(ADRY_RAM, y);
    lcdWriteIndex(DATA_RAM);
    for (uint32_t i = 0; i < (w+1) * (h+1); i++)
    {
        lcdWriteData(color);
    }
    reset_window();
}

void draw_poly(Point *points, uint32_t n, uint16_t color)
{
    for (uint32_t i = 0; i < n - 1; i++)
    {
        draw_line(points[i].x, points[i].y, points[i + 1].x, points[i + 1].y, color);
    }
}

volatile int32_t offset_y = -5, offset_x = -20;
volatile int32_t scale_y = 870, scale_x = 660;

void TP_get_mean_XY(volatile uint32_t *x, volatile uint32_t *y)
{
    uint32_t samples = 50;
    *x = 0;
    *y = 0;
    for (uint32_t i = 0; i < samples; i++)
    {
        int32_t tx = 0, ty = 0;
        touchpanelGetXY(&tx, &ty);
        *x += tx;
        *y += ty;
    }
    *x /= samples;
    *y /= samples;
}

void TP_config(void)
{
    fill_screen_fast(LCDBlack);
    uint32_t x1 = 40, y1 = 40;
    draw_line(x1 - 10, y1 - 10, x1 + 10, y1 + 10, LCDMagenta);
    draw_line(x1 - 10, y1 + 10, x1 + 10, y1 - 10, LCDMagenta);
    uint32_t tx1, ty1;
    // wait for tp irq
    while (LPC_GPIO0->FIOPIN & (1 << 19))
        ;
    TP_get_mean_XY(&tx1, &ty1);

	char buff[64];
	sprintf(buff, "%d, %d\r\n", tx1, ty1);
	UART_write_string(buff);
	
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
	
	sprintf(buff, "%d, %d\r\n", tx2, ty2);
	UART_write_string(buff);
	
    fill_screen_fast(LCDBlueSea);
    SYSTICK_wait(1000);


    scale_x = (x2 - x1) * 10000 / (ty2 - ty1);
    scale_y = (y2 - y1) * 10000 / (tx2 - tx1);

	sprintf(buff, "%d, %d\r\n", scale_x, scale_y);
	UART_write_string(buff);
	
    offset_x = x1 - scale_x * ty1 / 10000;
    offset_y = y1 - scale_y * tx1 / 10000;
	
	sprintf(buff, "%d, %d\r\n", offset_x, offset_y);
	UART_write_string(buff);
}

void TP_to_LCD(uint32_t tp_x, uint32_t tp_y, uint32_t *lcd_x, uint32_t *lcd_y) 
{
	*lcd_x = tp_y * scale_x / 10000 + offset_x;
	*lcd_y = tp_x * scale_y / 10000 + offset_y;
}
