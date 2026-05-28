#include "lcd_lib/LCD_ILI9325.h"
#include "lcd_lib/Open1768_LCD.h"
#include "lcd_lib/asciiLib.h"
#include "calligraphy.h"
#include "lcd.h"

void draw_char(uint8_t chr, uint16_t x, uint16_t y, uint16_t color, uint16_t bg_color) 
{
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
    for (uint32_t i = 0; i < 16; i++)
    {
        for (uint32_t j = 0; j < 8; j++)
        { 
            uint8_t mask = 1 << (7 - j);
            //fix_color(lcdReadData())
            lcdWriteData(mask & ascii_char[i] ? color : bg_color);
        }
    }

    reset_window();
}

void draw_text(const char *str, uint16_t x, uint16_t y, uint16_t color, uint16_t bg_color)
{
    while (*str) 
    {
        draw_char(*str, x, y, color, bg_color);
        str++;
        x += 8;
    }
}

void draw_char_vertical(uint8_t chr, uint16_t x, uint16_t y, uint16_t color, uint16_t bg_color)
{
	// char is 8x16 (w x h)
	// top to bottom line by line
	uint8_t ascii_char[16];
	GetASCIICode(1, ascii_char, chr);

	lcdWriteReg(HADRPOS_RAM_START, x); // horizontal is bottom top direction
	lcdWriteReg(HADRPOS_RAM_END, x + 15); // 16 is the height of the letter
	lcdWriteReg(VADRPOS_RAM_START, y); // vertical is left to right direction
	lcdWriteReg(VADRPOS_RAM_END, y + 7); // 8 is width of the letter

	lcdWriteReg(ADRX_RAM, x); // cursor starting x position 
	lcdWriteReg(ADRY_RAM, y); // cursor starting y position
	lcdWriteIndex(DATA_RAM); // DATA_RAM is interface register to lcd GRAM

	for (uint32_t i = 0; i < 16; i++)
	{
		for (uint32_t j = 0; j < 8; j++)
		{
			uint8_t mask = 1 << (7 - j);
            // writting letter from the bottom
            lcdWriteData(mask & ascii_char[15-i] ? color : bg_color);
		}
	}

	reset_window();
}

void draw_text_vertical(const char *str, uint16_t x, uint16_t y, uint16_t color, uint16_t bg_color) 
{
    // 7.2.5. Entry Mode (R03h)
	// ILI9325 Version 0.43 page 54
	// ENTRYM is write-only

	// Default settings for lcd writing increment
	// AM is 0 what means that horizontal axis is main axis to increment/decrement
	uint16_t id = (3 << 4); // 11 means horizontal increment and vertical increment
	uint16_t org = (1 << 7); // 1 means autoincrement ON and works in according to I/D[1:0] setting
	// BGR is 0 to follow the RGB order to write the pixel data and not BGR
	// TRI is 0
	// DFM irrelevant when TRI=0
	uint16_t default_entry = (0 | id | org);

	// Settings for lcd vertical writing
	uint16_t am = (1 << 3); // vertical axis is main axis
	id = (3 << 4); // 11 means horizontal increment and vertical increment
	org = (1 << 7); // 1 means autoincrement ON and works in according to I/D[1:0] setting
	uint16_t this_entry = (0 | am | id | org);

	lcdWriteReg(ENTRYM, this_entry);

	while (*str)
	{
		draw_char_vertical(*str, x, y, color, bg_color);
		str++;
		y += 8; // next letter on y axis
	}
    lcdWriteReg(ENTRYM, default_entry);
}
