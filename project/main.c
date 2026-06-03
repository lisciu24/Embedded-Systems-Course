#include "LPC17xx.h"
#include "generator_ctrl.h"
#include "interface.h"
#include "lcd.h"
#include "systick.h"
#include "uart.h"
#include "waves_lut.h"

void EINT0_init(void) {
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
    // UART_write_string("Click!\r\n");
    // if (flag == 0) {
    // 	GENCTRL_function(sin_lut, 1000, 5000);
    // } else {
    // 	GENCTRL_function(triangle_lut, MAX_AMPLITUDE, 10000);
    // }
    // flag ^= 1;
    TP_config();
    TP_config_store();
}

int main() {
    SYSTICK_init();

    UART_init_reg();
    UART_write_string("START\n\r");

    GENCTRL_init();
    GENCTRL_function(sin_lut, 1000, 5000);
    EINT0_init();

    lcdConfiguration();
    init_ILI9325();
    touchpanelInit();

    TP_config_restore();

    init_interface();

    for (;;) {
        while (LPC_GPIO0->FIOPIN & (1 << 19)) {
        }

        Point tp = TP_get_mean_XY();
        Point lcd = TP_to_LCD(tp);

        char buf[64];
        sprintf(buf, "lx: %d\tly: %d\r\n", lcd.x, lcd.y);
        UART_write_string(buf);

        // touch check inside drawing area
        if (lcd.x >= DRAW_MIN_X && lcd.x <= DRAW_MAX_X && lcd.y >= DRAW_MIN_Y &&
            lcd.y <= DRAW_MAX_Y) {
            UART_write_string("DRAW\r\n");
            draw_pixel(lcd.x, lcd.y, LCDBlack);
        }
    }
}
