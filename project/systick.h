#ifndef SYSTICK_H
#define SYSTICK_H

#include "LPC17xx.h"

volatile uint32_t ticks = 0; 
void SysTick_Handler(void) {
	ticks++;
}

void SYSTICK_wait(uint32_t _ticks) {
	uint32_t stop = ticks + _ticks;
	while(ticks != stop) 
		continue;
}

void SYSTICK_init(void) {
	SystemInit();
	// set interrupt to 1 ms
	SysTick_Config(SystemCoreClock / 1000);
}

#endif //SYSTICK_H
