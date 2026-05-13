#ifndef READ_GRAPH_H
#define READ_GRAPH_H

#include "LPC17xx.h"
#include "lcd.h"

#define DRAW_MIN_X 30
#define DRAW_MAX_X (LCD_MAX_X - 59)
#define DRAW_MIN_Y 30
#define DRAW_MAX_Y (LCD_MAX_Y - 30)

#define ERASE_B_MIN_X (DRAW_MAX_X + 5)
#define ERASE_B_MAX_X (ERASE_B_MIN_X + 24)
#define ERASE_B_MIN_Y (DRAW_MIN_Y)
#define ERASE_B_MAX_Y (ERASE_B_MIN_Y + 50)

#define FIX_B_MIN_X (ERASE_B_MIN_X)
#define FIX_B_MAX_X (FIX_B_MIN_X + 24)
#define FIX_B_MIN_Y (ERASE_B_MAX_Y + 5)
#define FIX_B_MAX_Y (FIX_B_MIN_Y + 34)

void read_graph(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end, uint16_t* values)
{
    //if (x_start < 0) x_start = 0;
    if (x_end > LCD_MAX_X) x_end = LCD_MAX_X;
    //if (y_start < 0) y_start = 0;
    if (y_end > LCD_MAX_Y) y_end = LCD_MAX_Y;

    int16_t temp_values[DRAW_MAX_Y - DRAW_MIN_Y];

    // finding max index of black pixel in column
    for (uint16_t y = y_start; y < y_end; y++) 
    {
        int16_t max_found_x = -1;
        for (uint16_t x = x_start; x < x_end; x++) 
        {
            uint16_t color = read_pixel_color(x,y);
            if (color == LCDBlack)
            {
                    max_found_x = x - x_start;
            }
        }
        temp_values[y - y_start] = max_found_x;
    }

    // linear interpolation  between points
    uint16_t w = y_end - y_start;
    for (uint16_t i = 0; i < w; i++) 
    {
        if (temp_values[i] == -1)
        {

            int l = i - 1;
            while (l >= 0 && temp_values[l] == -1) l--;

            int r = i + 1;
            while (r < w && temp_values[r] == -1) r++;

            if (l >= 0 && r < w)
            {
                // linear interpolation between left and right found
                int16_t left_val = temp_values[l];
                int16_t right_val = temp_values[r];

                for (int k = l + 1; k < r; k++)
                {
                    // interpolation equation
                    temp_values[k] = left_val +  (k - l) * ((right_val - left_val) / (r - l));
                }

                i = r; // jump to the one before lacking value
            }
            else if (l >= 0)
            {
                // there is no right so copy left
                for (int k = l + 1; k < w; k++)
                {
                    temp_values[k] = temp_values[l];
                    i = k;
                }
            }
            else if (r < w)
            {
                // there is no left so copy right
                for (int k = 0; k < r; k++)
                {
                    temp_values[k] = temp_values[r];
                }
                i = r;
            }
        }
    }

  for (uint16_t y = y_start; y < y_end; y++) values[y - y_start] = temp_values[y - y_start];
}

/*
int main()
{
    draw_rect(DRAW_MIN_X - 1, DRAW_MIN_Y - 1, DRAW_MAX_X -  DRAW_MIN_X + 2, DRAW_MAX_Y -  DRAW_MIN_Y + 2, LCDRed);// drawing board
    fill_rect(ERASE_B_MIN_X, ERASE_B_MIN_Y, ERASE_B_MIN_X - ERASE_B_MIN_X, ERASE_B_MAX_Y - ERASE_B_MIN_Y, LCDMagenta);// erase button
    const char *erase_str = "ERASE";
    draw_text(erase_str, ERASE_B_MIN_X + 4, ERASE_B_MIN_Y + 5, LCDBlack);
    fill_rect(FIX_B_MIN_X, FIX_B_MIN_Y, FIX_B_MIN_X - FIX_B_MIN_X, FIX_B_MAX_Y - FIX_B_MIN_Y, LCDGreen);// fix the graph button
    const char *fix_str = "FIX";
    draw_text(fix_str, FIX_B_MIN_X, FIX_B_MIN_Y, LCDBlack);

    uint16_t erasing = 0;
    uint16_t fix = 0;
    for (;;) {
        uint32_t tx = 0, ty = 0;
        while (LPC_GPIO0->FIOPIN & (1 << 19))
            ;
        TP_get_mean_XY(&tx, &ty);
        
        uint32_t lx = 0, ly = 0;
        TP_to_LCD(tx, ty, &lx, &ly);

        if (lx >= ERASE_B_MIN_X && lx <= ERASE_B_MAX_X && ly >= ERASE_B_MIN_Y && ly <= ERASE_B_MAX_Y)
        {
            erasing = 1;
        }

        if (lx >= FIX_B_MIN_X && lx <= FIX_B_MAX_X && ly >= FIX_B_MIN_Y && ly <= FIX_B_MAX_Y)
        {
            fix = 1;
        }
        
        if (lx >= DRAW_MIN_X && lx <= DRAW_MAX_X && ly >= DRAW_MIN_Y && ly <= DRAW_MAX_Y)
        {
            draw_pixel(lx, ly, LCDBlack);
        }

        if (fix == 1)
        {
            uint16_t values[DRAW_MAX_X - DRAW_MIN_X];
            values = read_graph(DRAW_MIN_X, DRAW_MAX_X, DRAW_MIN_Y, DRAW_MAX_Y, values);
            draw_rect(DRAW_MIN_X, DRAW_MIN_Y, DRAW_MAX_X - DRAW_MIN_X, DRAW_MAX_Y - DRAW_MIN_Y, LCDWhite);
            for (uint16_t x = DRAW_MIN_X; x < DRAW_MAX_X; x++)
            {
                draw_pixel(x, values[x - DRAW_MIN_X] + DRAW_MIN_Y, LCDBlack);
            }
            fix = 0;
        }
        
        if (erasing == 1)
        {
            draw_rect(DRAW_MIN_X, DRAW_MIN_Y, DRAW_MAX_X - DRAW_MIN_X, DRAW_MAX_Y - DRAW_MIN_Y, LCDWhite);
            erasing = 0
        }

    }
}
*/

#endif //READ_GRAPH_H
