#include "interface.h"
#include "LPC17xx.h"
#include "calligraphy.h"
#include "lcd.h"
#include "systick.h"
#include "uart.h"
#include "generator_ctrl.h"
#include <stdbool.h>

uint32_t lcd_graph;

// draws button using provided Button struct
// with_text == true draws text in the button
// text_vertical == true draws text vertically on the screen (from lower to
// higher y)
void button_draw(Button *btn, bool with_text, bool text_vertical) {
    fill_rect(btn->x, btn->y, btn->width, btn->height, btn->bg_color);

    if (with_text) {
        if (text_vertical) {
            //draw_text_vertical(btn->label, btn->x + 4, btn->y + 5,
                               //btn->text_color, btn->bg_color);
        } else {
            //draw_text(btn->label, btn->x + 5, btn->y + 4, btn->text_color,
                      //btn->bg_color);
        }
    }
}

bool button_contains(Button *btn, uint16_t px, uint16_t py) {
    return (px >= btn->x && px < (btn->x + btn->width) && py >= btn->y &&
            py < (btn->y + btn->height));
}

void button_handle_touch(Button *btn, uint16_t px, uint16_t py) {
    if (button_contains(btn, px, py)) {
        if (btn->on_click != NULL) {
            btn->on_click();
        }
    }
}

void erase_button_callback(void) {
    UART_write_string("ERASE\r\n");

    fill_rect(DRAW_MIN_X, DRAW_MIN_Y, DRAW_MAX_X - DRAW_MIN_X + 1,
              DRAW_MAX_Y - DRAW_MIN_Y + 1, LCDWhite);
}

void fix_button_callback(void) {
    UART_write_string("FIX\r\n");

    uint16_t values[DRAW_MAX_Y - DRAW_MIN_Y + 1];

    read_graph(DRAW_MIN_X, DRAW_MAX_X, DRAW_MIN_Y, DRAW_MAX_Y, values);

    fill_rect(DRAW_MIN_X, DRAW_MIN_Y, DRAW_MAX_X - DRAW_MIN_X + 1,
              DRAW_MAX_Y - DRAW_MIN_Y + 1, LCDWhite);

    for (uint16_t y = DRAW_MIN_Y; y <= DRAW_MAX_Y; y++) {
        draw_pixel(values[y - DRAW_MIN_Y] + DRAW_MIN_X, y, LCDBlack);
    }
}

void dac_button_callback(void) {
    UART_write_string("DAC\r\n");
	uint16_t values[DRAW_MAX_Y - DRAW_MIN_Y + 1];
	read_graph(DRAW_MIN_X, DRAW_MAX_X, DRAW_MIN_Y, DRAW_MAX_Y, values);
	
	uint16_t lut[FSAMPLE];
	uint16_t step = (DRAW_MAX_Y - DRAW_MIN_Y + 1) * 1000 / 50;
	uint16_t max_v = DRAW_MAX_X - DRAW_MIN_X;
	for (uint16_t i = 0; i < 50; i++) {
		lut[i] = (values[i * step / 1000] * 1023U) / max_v;
	}
	
	GENCTRL_function(lut, MAX_AMPLITUDE, 10000);
}

// https://www.geeksforgeeks.org/c/how-to-initialize-structures-in-c/
// Designated Initialization
Button erase_button = {.x = ERASE_B_MIN_X,
                       .y = ERASE_B_MIN_Y,
                       .width = ERASE_B_MAX_X - ERASE_B_MIN_X + 1,
                       .height = ERASE_B_MAX_Y - ERASE_B_MIN_Y + 1,

                       .bg_color = LCDMagenta,
                       .text_color = LCDBlack,

                       .label = "ERASE",

                       .on_click = erase_button_callback};

Button fix_button = {.x = FIX_B_MIN_X,
                     .y = FIX_B_MIN_Y,
                     .width = FIX_B_MAX_X - FIX_B_MIN_X + 1,
                     .height = FIX_B_MAX_Y - FIX_B_MIN_Y + 1,

                     .bg_color = LCDGreen,
                     .text_color = LCDBlack,

                     .label = "FIX",

                     .on_click = fix_button_callback};

Button dac_button = {.x = DAC_B_MIN_X,
                     .y = DAC_B_MIN_Y,
                     .width = DAC_B_MAX_X - DAC_B_MIN_X + 1,
                     .height = DAC_B_MAX_Y - DAC_B_MIN_Y + 1,

                     .bg_color = LCDCyan,
                     .text_color = LCDBlack,

                     .label = "DAC",

                     .on_click = dac_button_callback};

Button *buttons[] = {&erase_button, &fix_button, &dac_button};

#define BUTTON_COUNT (sizeof(buttons) / sizeof(buttons[0]))

// values returned are in range [0, (DRAW_MAX_X - DRAW_MIN_X)]
void read_graph(uint16_t x_start, uint16_t x_end, uint16_t y_start,
                uint16_t y_end, uint16_t values[]) {
    if (x_end > LCD_MAX_X)
        x_end = LCD_MAX_X;
    if (y_end > LCD_MAX_Y)
        y_end = LCD_MAX_Y;

    int16_t temp_values[DRAW_MAX_Y - DRAW_MIN_Y + 1];

    // finding max index of black pixel in column
    for (uint16_t y = y_start; y <= y_end; y++) {
        int16_t max_found_x = -1;
        for (uint16_t x = x_start; x <= x_end; x++) {
            uint16_t color = read_pixel_color(x, y);
            if (color == LCDBlack) {
                max_found_x = x - x_start;
            }
        }
        temp_values[y - y_start] = max_found_x;
    }

    // linear interpolation  between points
    uint16_t w = y_end - y_start + 1;
    for (uint16_t i = 0; i < w; i++) {
        if (temp_values[i] == -1) {

            int l = i - 1;
            while (l >= 0 && temp_values[l] == -1)
                l--;

            int r = i + 1;
            while (r < w && temp_values[r] == -1)
                r++;

            if (l >= 0 && r < w) {
                // linear interpolation between left and right found
                int16_t left_val = temp_values[l];
                int16_t right_val = temp_values[r];

                for (int k = l + 1; k < r; k++) {
                    // interpolation equation
                    temp_values[k] =
                        left_val + (k - l) * (right_val - left_val) / (r - l);
                }

                i = r; // jump to the one before lacking value
            } else if (l >= 0) {
                // there is no right so copy left
                for (int k = l + 1; k < w; k++) {
                    temp_values[k] = temp_values[l];
                    i = k;
                }
            } else if (r < w) {
                // there is no left so copy right
                for (int k = 0; k < r; k++) {
                    temp_values[k] = temp_values[r];
                }
                i = r;
            }
        }
    }

    for (uint16_t y = y_start; y <= y_end; y++)
        values[y - y_start] = temp_values[y - y_start];
}

void init_interface(void) {
    fill_screen_fast(LCDWhite);

    // red border of the drawing area
    draw_rect(DRAW_MIN_X - 1, DRAW_MIN_Y - 1, DRAW_MAX_X - DRAW_MIN_X + 3,
              DRAW_MAX_Y - DRAW_MIN_Y + 3, LCDRed);

    // drawing all buttons
    for (uint32_t i = 0; i < BUTTON_COUNT; i++) {
        button_draw(buttons[i], true, true);
    }

    for (;;) {
        while (LPC_GPIO0->FIOPIN & (1 << 19)) {
        }

        Point tp = TP_get_mean_XY();
        Point lcd = TP_to_LCD(tp);

		char buf[64];
		sprintf(buf, "lx: %d\tly: %d\r\n", lcd.x, lcd.y);
		UART_write_string(buf);
		
		
        // check if any button is clicked
        for (uint32_t i = 0; i < BUTTON_COUNT; i++) {
            button_handle_touch(buttons[i], lcd.x, lcd.y);
        }
		

        // touch check inside drawing area
        if (lcd.x >= DRAW_MIN_X && lcd.x <= DRAW_MAX_X && lcd.y >= DRAW_MIN_Y &&
            lcd.y <= DRAW_MAX_Y) {
			UART_write_string("DRAW\r\n");
            draw_pixel(lcd.x, lcd.y, LCDBlack);
        }
    }
}
