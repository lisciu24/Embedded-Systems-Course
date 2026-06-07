#include "systick.h"

volatile uint32_t ticks = 0; 
void SysTick_Handler(void) {
	ticks++;
}

void SYSTICK_wait(uint32_t delay) {
	uint32_t start = ticks;
	while((ticks - start) < delay)
		;
}

void SYSTICK_init(void) {
	SystemInit();
	// set interrupt to 1 ms
	SysTick_Config(SystemCoreClock / 1000);
	NVIC_SetPriority(SysTick_IRQn, 0);
}
