#include "LPC17xx.h"
#include "uart.h"
#include "interface.h"
#include "systick.h"

// DAC has 10 bit resolution so value range [0,1023]
// DACR has DAC value on [15:6]
#define DACV(x) (x << 6)
#define FSAMPLE 3

volatile uint32_t scale = 1;
volatile uint32_t freq = 0;

uint16_t values[][3] = {
	{DACV(0x0000), DACV(0x01FF), DACV(0x03FF)},
	{DACV(0x0000), DACV(0x03FF), DACV(0x0000)},
};
volatile uint32_t values_idx = 0;

typedef struct LLI_s {
	uint32_t DMACCSrcAddr;
	uint32_t DMACCDestAddr;
	struct LLI_s *DMACCLLI;
	uint32_t DMACCControl;
} LLI_t;

LLI_t LLI;

void LLI_init(uint32_t fun_idx)
{
	LLI.DMACCLLI = &LLI;
	// Set source address for LLI
	LLI.DMACCSrcAddr = (uint32_t)values[fun_idx];
	// Set destination address for LLI
	LLI.DMACCDestAddr = (uint32_t)&LPC_DAC->DACR;
	LLI.DMACCControl = FSAMPLE | 1 << 18 | 1 << 21 | 1 << 26;
}

void DAC_init(void)
{
	// Set P0.26 to AOUT function, enables DAC
	LPC_PINCON->PINSEL1 &= ~(0b11 << 20);
	LPC_PINCON->PINSEL1 |= 0b10 << 20;

	// Enable DMA, double buffering, time-out counter
	LPC_DAC->DACCTRL = 0b0110;

	// pclk 25MHz, max update rate 1MHz
	// 16-bit timer, allowed values 25 and bigger
	LPC_DAC->DACCNTVAL = 100U;
}

void DMA_init(uint32_t fun_idx)
{
	LLI_init(fun_idx);
	LPC_SC->PCONP |= 1 << 29;            // Power up DMA
	LPC_GPDMA->DMACConfig = 0b01;        // Enable DMA controller
	LPC_GPDMA->DMACIntErrClr |= 1 << 0;  // Clear error interrupt
	LPC_GPDMA->DMACIntTCClear |= 1 << 0; // Clear transcation finished interrupt
	// Set source address for chanel 0
	LPC_GPDMACH0->DMACCSrcAddr = (uint32_t)values[fun_idx];
	// Set destination address for chanel 0
	LPC_GPDMACH0->DMACCDestAddr = (uint32_t)&LPC_DAC->DACR;
	// Set LLI address
	LPC_GPDMACH0->DMACCLLI = (uint32_t)&LLI;

	// Sets: TransferSize to values_size,
	// SWidth to 16-bit, DWidth to 16-bit,
	// Source increment
	LPC_GPDMACH0->DMACCControl = FSAMPLE | 1 << 18 | 1 << 21 | 1 << 26 | 1U << 31;

	// Enables DMA channel,
	// Sets: DestPeripheral to DAC,
	// TransferType memory to peripheral
	LPC_GPDMACH0->DMACCConfig = 1 | 0x07 << 6 | 0b001 << 11 | 0b11 << 14;
	NVIC_EnableIRQ(DMA_IRQn);
}

void DMA_stop(void)
{
	LPC_GPDMACH0->DMACCConfig |= 1 << 18;
	while(LPC_GPDMACH0->DMACCConfig & 1 << 17)
		;
	LPC_GPDMACH0->DMACCConfig &= ~1;
}

void DMA_IRQHandler(void)
{
	UART_write_string("DMA\n\r");
}

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

volatile uint32_t lcd_graph = 0; // 1 if dac graph is drawn from lcd

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
	
	init_interface();
	
	for(;;) {}
}
