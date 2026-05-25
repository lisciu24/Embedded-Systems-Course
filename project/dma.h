#ifndef DMA_H
#define DMA_H

#include "LPC17xx.h"
#include "uart.h"

// DAC has 10 bit resolution so value range [0,1023]
// DACR has DAC value on [15:6]
#define DACV(x) (x << 6)
#define FSAMPLE 3

typedef struct LLI_s {
	uint32_t DMACCSrcAddr;
	uint32_t DMACCDestAddr;
	struct LLI_s *DMACCLLI;
	uint32_t DMACCControl;
} LLI_t;

void LLI_init(uint32_t fun_idx);
void DAC_init(void);
void DMA_init(uint32_t fun_idx);
void DMA_IRQHandler(void);
void DMA_stop(void);

#endif // DMA_H
