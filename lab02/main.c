#include "LPC17xx.h"   // Device header
#include "Board_LED.h" // ::Board Support:LED
#include "GPIO_LPC17xx.h"               // Keil::Device:GPIO
#include "PIN_LPC17xx.h"                // Keil::Device:PIN
#include "Board_Buttons.h"              // ::Board Support:Buttons



volatile uint32_t ticks = 0;
volatile uint32_t pwm_ticks = 0;

volatile uint32_t ledv = 0;
volatile uint32_t led_dimm = 0;

void SysTick_Handler(void) {
	pwm_ticks++;
	pwm_ticks = pwm_ticks % 100;
	
	if(led_dimm < pwm_ticks) {
		GPIO_PinWrite(0, 3, 0U);
		LED_SetOut(0);
	} else {
		GPIO_PinWrite(0, 3, 1U);
		LED_SetOut(ledv);
	}
	
	ticks++;
}

void LED_dimm(uint32_t dimm) {
	led_dimm = dimm;
}

void brute_wait(uint32_t loops) {
	for(uint32_t j = 0; j < loops; j++)
		for(uint32_t i = 0; i < loops; i++)
			continue;
}

void wait(uint32_t _ticks) {
	uint32_t stop = ticks + _ticks * 100;
	while(ticks != stop) 
		continue;
}

uint32_t check(uint32_t* _start, uint32_t _ticks) {
	if(*_start == 0) *_start = ticks;
	
	if(100 * _ticks <= ticks - *_start) {
		*_start = ticks;
		return 1;
	}
		
	return 0;
}

void setup(void) {
	SystemInit();
	SysTick_Config(SystemCoreClock / 100000);
	
	PIN_Configure (0, 3, PIN_FUNC_0, PIN_PINMODE_PULLDOWN, PIN_PINMODE_NORMAL);
    GPIO_SetDir   (0, 3, GPIO_DIR_OUTPUT);
    GPIO_PinWrite (0, 3, 0U);
	
	LED_Initialize();
	LED_SetOut(0);
	
	Buttons_Initialize();
}

void loop(void) {
	uint32_t led_count = LED_GetCount();
	uint32_t ledv_stop = (1 << led_count) - 1;
	ledv = 0b101;
	uint32_t i = 0;
	
	
	uint32_t animate_flag = 1;
	uint32_t animate_ticks = 100;
	uint32_t animate_start = 0;
	
	uint32_t dimm_ticks = 10;
	uint32_t dimm_start = 0;
	
	uint32_t button_flag = 0;
	uint32_t button_ticks = 20;
	uint32_t button_start = 0;
	
	for(;;) {
		if(check(&animate_start, animate_ticks) && animate_flag) {
			ledv <<= 1;
			ledv = ledv % ledv_stop;
		}
		
		if(check(&dimm_start, dimm_ticks)) {
			LED_dimm(i);
			i++;
			i = i % 100;
		}
		
		if(check(&button_start, button_ticks))
		{
			if(Buttons_GetState()) {
				if(!button_flag) {
					button_flag = true;
					animate_flag = !animate_flag;
				}
			} else {
				button_flag = false;
			}
		}

	}
}

int main() {
	setup();
	while(1)
		loop();
}

