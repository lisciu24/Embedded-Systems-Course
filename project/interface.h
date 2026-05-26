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


// https://www.geeksforgeeks.org/c/how-to-create-typedef-for-function-pointer-in-c/
typedef void (*ButtonCallback)(void); // Typedef for a Function Pointer in C


typedef struct
{
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;

    uint16_t bg_color;
    uint16_t text_color;

    const char *label;

    ButtonCallback on_click;

} Button;

void button_draw(Button *btn, bool with_text, bool text_vertical);
bool button_contains(Button *btn, uint16_t px, uint16_t py);
void button_handle_touch(Button *btn, uint16_t px, uint16_t py);
void erase_button_callback(void);
void fix_button_callback(void);
void dac_button_callback(void);
void read_graph(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end, uint16_t values[]);
void init_interface(void);

#endif //INTERFACE_H
