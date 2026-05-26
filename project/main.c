#include "LPC17xx.h"
#include "uart.h"
#include "interface.h"
#include "systick.h"
#include "dma.h"
#include "lcd.h"

volatile uint32_t scale = 1;
volatile uint32_t freq = 0;
volatile uint32_t values_idx = 0;

void EINT0_init(void)
{
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

uint32_t lcd_graph = 0; // 1 if dac graph is drawn from lcd

void EINT0_IRQHandler(void)
{
	LPC_SC->EXTINT = 0x01; // Clear EINT0 interrupt flag
	UART_write_string("Click!\r\n");
	lcd_graph = 0;
	DMA_init(values_idx);
	values_idx = (values_idx + 1) % 2;
}

int main() {
	
	SYSTICK_init();
	
	EINT0_init();
	UART_init_reg();
	UART_write_string("START\n\r");
	DAC_init();	
	
	lcdConfiguration();
	init_ILI9325();
	touchpanelInit();
	
	//TP_config();
	init_interface();
	//fill_screen_fast(LCDBlueSea);

	for(;;) {}
}
