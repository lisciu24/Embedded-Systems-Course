#include "dma.h"

uint16_t values[][FSAMPLE] = {
	{DACV(0x0000), DACV(0x01FF), DACV(0x03FF)},
	{DACV(0x0000), DACV(0x03FF), DACV(0x0000)},
};

LLI_t LLI;

void LLI_init(uint32_t fun_idx)
{
	LLI.DMACCLLI = &LLI;
	// Set source address for LLI
	LLI.DMACCSrcAddr = (uint32_t)values[fun_idx];
	// Set destination address for LLI
	LLI.DMACCDestAddr = (uint32_t)&LPC_DAC->DACR;
	LLI.DMACCControl = FSAMPLE | 1 << 18 | 1 << 21 | 1 << 26;// | 1U << 31;
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

void DMA_IRQHandler(void)
{
	LPC_GPDMA->DMACIntErrClr |= 1 << 0;  // Clear error interrupt
	LPC_GPDMA->DMACIntTCClear |= 1 << 0; // Clear transcation finished interrupt
	UART_write_string("DMA\n\r");
}

void DMA_stop(void)
{
	LPC_GPDMACH0->DMACCConfig |= 1 << 18;
	while(LPC_GPDMACH0->DMACCConfig & 1 << 17)
		;
	LPC_GPDMACH0->DMACCConfig &= ~1;
}
