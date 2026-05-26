#ifndef SYSTICK_H
#define SYSTICK_H

#include "LPC17xx.h"

void SysTick_Handler(void);
void SYSTICK_wait(uint32_t delay);
void SYSTICK_init(void);

#endif //SYSTICK_H
