#include "interface.h"
#include "LPC17xx.h"
#include "lcd.h"
#include "calligraphy.h"
#include "dma.h"
#include "systick.h"

// values returned are in range [0, (DRAW_MAX_X - DRAW_MIN_X)]
void read_graph(uint16_t x_start, uint16_t x_end, uint16_t y_start, uint16_t y_end, uint16_t values[])
{
    if (x_end > LCD_MAX_X) x_end = LCD_MAX_X;
    if (y_end > LCD_MAX_Y) y_end = LCD_MAX_Y;

    int16_t temp_values[DRAW_MAX_Y - DRAW_MIN_Y + 1];

    // finding max index of black pixel in column
    for (uint16_t y = y_start; y <= y_end; y++)
    {
        int16_t max_found_x = -1;
        for (uint16_t x = x_start; x <= x_end; x++) 
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
    uint16_t w = y_end - y_start + 1;
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

  for (uint16_t y = y_start; y <= y_end; y++) values[y - y_start] = temp_values[y - y_start];
}

extern uint32_t lcd_graph;

void init_interface(void) {
	fill_screen_fast(LCDWhite);
	draw_rect(DRAW_MIN_X - 1, DRAW_MIN_Y - 1, DRAW_MAX_X -  DRAW_MIN_X + 3, DRAW_MAX_Y -  DRAW_MIN_Y + 3, LCDRed);// drawing board
    fill_rect(ERASE_B_MIN_X, ERASE_B_MIN_Y, ERASE_B_MAX_X - ERASE_B_MIN_X + 1, ERASE_B_MAX_Y - ERASE_B_MIN_Y + 1, LCDMagenta);// erase button
    const char *erase_str = "ERASE";
    //draw_text_vertical(erase_str, ERASE_B_MIN_X + 4, ERASE_B_MIN_Y + 5, LCDBlack, LCDMagenta);
    fill_rect(FIX_B_MIN_X, FIX_B_MIN_Y, FIX_B_MAX_X - FIX_B_MIN_X + 1, FIX_B_MAX_Y - FIX_B_MIN_Y + 1, LCDGreen);// fix the graph button
    const char *fix_str = "FIX";
    //draw_text_vertical(fix_str, FIX_B_MIN_X + 4, FIX_B_MIN_Y + 5, LCDBlack, LCDGreen);
    fill_rect(DAC_B_MIN_X, DAC_B_MIN_Y, DAC_B_MAX_X - DAC_B_MIN_X + 1, DAC_B_MAX_Y - DAC_B_MIN_Y + 1, LCDCyan);// DAC button
    const char *dac_str = "DAC";
    //draw_text_vertical(dac_str, DAC_B_MIN_X + 4, DAC_B_MIN_Y + 5, LCDBlack, LCDGreen);

    uint16_t erasing = 0;
    uint16_t fix = 0;
    for (;;) {
		UART_write_string("LOOP\r\n");
        uint32_t tx = 0, ty = 0;
        while (LPC_GPIO0->FIOPIN & (1 << 19))
            ;
        TP_get_mean_XY(&tx, &ty);
        
        uint32_t lx = 0, ly = 0;
        TP_to_LCD(tx, ty, &lx, &ly);
		
		char buff[64];
		sprintf(buff, "%ud, %ud, %ud, %ud\r\n", tx, ty, lx, ly);
		UART_write_string(buff);

        if (lx >= ERASE_B_MIN_X && lx <= ERASE_B_MAX_X && ly >= ERASE_B_MIN_Y && ly <= ERASE_B_MAX_Y)
        {
            erasing = 1;
        }

        if (lx >= FIX_B_MIN_X && lx <= FIX_B_MAX_X && ly >= FIX_B_MIN_Y && ly <= FIX_B_MAX_Y)
        {
            fix = 1;
        }
		
        if (lx >= DAC_B_MIN_X && lx <= DAC_B_MAX_X && ly >= DAC_B_MIN_Y && ly <= DAC_B_MAX_Y)
        {
            lcd_graph = 1;
        }
        
        if (lx >= DRAW_MIN_X && lx <= DRAW_MAX_X && ly >= DRAW_MIN_Y && ly <= DRAW_MAX_Y)
        {
			UART_write_string("DRAW\r\n");
            draw_pixel((uint16_t)lx, (uint16_t)ly, LCDBlack);
        }

        if (lcd_graph == 1)
        {
			UART_write_string("GRAPH\r\n");
            uint16_t values[DRAW_MAX_Y - DRAW_MIN_Y + 1];
			
            read_graph(DRAW_MIN_X, DRAW_MAX_X, DRAW_MIN_Y, DRAW_MAX_Y, values);
            draw_rect(DRAW_MIN_X, DRAW_MIN_Y, DRAW_MAX_X - DRAW_MIN_X + 1, DRAW_MAX_Y - DRAW_MIN_Y + 1, LCDWhite);
            for (uint16_t x = DRAW_MIN_X; x <= DRAW_MAX_X; x++)
            {
                draw_pixel(x, values[x - DRAW_MIN_X] + DRAW_MIN_Y, LCDBlack);
            }
			
            DMA_stop();
            // DAC has 10 bit resolution so value range [0,1023]
            // DACR has DAC value on [15:6]
            uint16_t min_v = 0; // minimum inclusive value on the graph
            uint16_t max_v = DRAW_MAX_X - DRAW_MIN_X; // maximum inclusive value on the graph
            // pclk 25MHz, max update rate of DAC is 1MHz
            while (lcd_graph)
            {
                for(uint16_t i = 0; i <= (DRAW_MAX_Y - DRAW_MIN_Y); i++)
                {
                    uint16_t dac_value = (values[i] * 1023U) / 150U;
                    LPC_DAC->DACR = DACV(dac_value);
                    // there is 259 values
                    //ms
                    SYSTICK_wait(1); // hardcoded frequency of DAC from lcd
                }
            }
        }

        if (fix == 1)
        {
			UART_write_string("FIX\r\n");
            uint16_t values[DRAW_MAX_Y - DRAW_MIN_Y + 1];
            read_graph(DRAW_MIN_X, DRAW_MAX_X, DRAW_MIN_Y, DRAW_MAX_Y, values);
            draw_rect(DRAW_MIN_X, DRAW_MIN_Y, DRAW_MAX_X - DRAW_MIN_X + 1, DRAW_MAX_Y - DRAW_MIN_Y + 1, LCDWhite);
            for (uint16_t x = DRAW_MIN_X; x <= DRAW_MAX_X; x++)
            {
                draw_pixel(x, values[x - DRAW_MIN_X] + DRAW_MIN_Y, LCDBlack);
            }
        
            fix = 0;
        }
        
        if (erasing == 1)
        {
			UART_write_string("ERASE\r\n");
            draw_rect(DRAW_MIN_X, DRAW_MIN_Y, DRAW_MAX_X - DRAW_MIN_X + 1, DRAW_MAX_Y - DRAW_MIN_Y + 1, LCDWhite);
            erasing = 0;
        }
    }
}
