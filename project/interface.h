#ifndef INTERFACE_H
#define INTERFACE_H

#include "LPC17xx.h"

#define DRAW_MIN_X 30
#define DRAW_MAX_X (LCD_MAX_X - 59)
#define DRAW_MIN_Y 30
#define DRAW_MAX_Y (LCD_MAX_Y - 30)

#define ERASE_B_MIN_X (DRAW_MAX_X + 5)
#define ERASE_B_MAX_X (ERASE_B_MIN_X + 24) //height of the letter (16 bits) + 4 bits top/bottom padding
#define ERASE_B_MIN_Y (DRAW_MIN_Y)
#define ERASE_B_MAX_Y (ERASE_B_MIN_Y + 50) //width of the 5 letters where each 8 bits + 5 bits left/right padding

#define FIX_B_MIN_X (ERASE_B_MIN_X)
#define FIX_B_MAX_X (FIX_B_MIN_X + 24) //height of the letter (16 bits) + 4 bits top/bottom padding
#define FIX_B_MIN_Y (ERASE_B_MAX_Y + 5)
#define FIX_B_MAX_Y (FIX_B_MIN_Y + 34) //width of the 3 letters where each 8 bits + 5 bits padding

#define DAC_B_MIN_X (ERASE_B_MIN_X)
#define DAC_B_MAX_X (FIX_B_MIN_X + 24) //height of the letter (16 bits) + 4 bits top/bottom padding
#define DAC_B_MIN_Y (FIX_B_MAX_Y + 5)
#define DAC_B_MAX_Y (DAC_B_MIN_Y + 34) //width of the 3 letters where each 8 bits + 5 bits padding

void read_graph(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end, uint16_t* values);
void init_interface(void);

#endif //INTERFACE_H
