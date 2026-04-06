#include "LPC17xx.h"
#include "uart.h"

volatile uint32_t ticks = 0; 
void SysTick_Handler(void) {
	ticks++;
}

void wait(uint32_t _ticks) {
	uint32_t stop = ticks + _ticks;
	while(ticks != stop) 
		continue;
}

void Timer_init(void) {
  LPC_SC->PCONP |= (1 << 1);        // Power up Timer0
  LPC_SC->PCLKSEL0 &= ~(0x03 << 2); // Clear PCLK_TIMER0
  LPC_SC->PCLKSEL0 |= (0x00 << 2);  // Set PCLK_TIMER0
  LPC_TIM0->TCR = 0x02;             // Reset Timer0
  LPC_TIM0->PR = 1e5 - 1;           // Prescaler
  LPC_TIM0->MR0 = 125;              // Match value
  LPC_TIM0->MCR = 0x03;             // Interrupt and reset on MR0
  LPC_TIM0->IR = 0x3F;              // Clear all interrupts
  LPC_TIM0->TCR = 0x01;             // Start Timer0
	NVIC_SetPriority(TIMER0_IRQn, 1);
  NVIC_EnableIRQ(TIMER0_IRQn);      // Enable Timer0 interrupt in NVIC
}

void TIMER0_IRQHandler(void) {
  LPC_TIM0->IR = 0x01; // Clear MR0 interrupt
  UART_write_string("Ping!\r\n");
}

volatile uint32_t RTC_flag = 0;

void RTC_init(void) {
  LPC_SC->PCONP |= (1 << 9); // Power up RTC
  LPC_RTC->CCR = 0x11;       // Enable RTC
  LPC_RTC->CIIR = 0x01;      // Interrupt on second increment
  LPC_RTC->AMR = 0xFF;       // Don't mask any value
  LPC_RTC->ILR = 0x03;       // Clear interrupt flag
	NVIC_SetPriority(RTC_IRQn, 1);
  NVIC_EnableIRQ(RTC_IRQn);  // Enable RTC interrupt in NVIC
}

void RTC_IRQHandler(void) {
  LPC_RTC->ILR = 0x01;  // Clear second interrupt flag
  RTC_flag = ~RTC_flag; // Toggle flag
  UART_write_string(RTC_flag ? "Tik!\r\n" : "Tak!\r\n");
}

void EINT0_init(void) {
  //led setup
  LPC_GPIO0->FIODIR |= 0x01;
  LPC_GPIO0->FIOPIN |= 0x01;
  // EINT setup
  LPC_SC->EXTINT = 0x01;                    // Clear EINT0 interrupt flag
  LPC_SC->EXTMODE |= 0x01;                  // Set EINT0 to edge-sensitive
  LPC_SC->EXTPOLAR &= ~0x01;                // Set EINT0 to rising edge
  LPC_SC->EXTINT = 0x01;                    // Clear EINT0 interrupt flag
  LPC_PINCON->PINSEL4 &= ~(0x03 << 20);     // Clear P2.10 function
  LPC_PINCON->PINSEL4 |= (0x01 << 20);      // Set P2.10 to EINT0
  LPC_PINCON->PINMODE4 &= ~(0x03 << 20);    // Set P2.10 to pull-up mode
  LPC_PINCON->PINMODE_OD2 &= ~(0x01 << 10); // Set P2.10 to normal mode
  LPC_SC->EXTINT = 0x01;                    // Clear EINT0 interrupt flag
	NVIC_SetPriority(EINT0_IRQn, 1);
  NVIC_EnableIRQ(EINT0_IRQn);               // Enable EINT0 interrupt in NVIC
}

void EINT0_IRQHandler(void) {
  LPC_SC->EXTINT = 0x01; // Clear EINT0 interrupt flag
  LPC_GPIO0->FIOPIN ^= 0x01;
  UART_write_string("Click!\r\n");
}

void EINT1_init(void) {
	//led setup
  LPC_GPIO0->FIODIR |= 0x02;
  LPC_GPIO0->FIOPIN |= 0x02;
  // EINT setup
  LPC_SC->EXTINT = 0x02;                    // Clear EINT0 interrupt flag
  LPC_SC->EXTMODE |= 0x02;                  // Set EINT0 to edge-sensitive
  LPC_SC->EXTPOLAR &= ~0x02;                // Set EINT0 to rising edge
  LPC_SC->EXTINT = 0x01;                    // Clear EINT0 interrupt flag
  LPC_PINCON->PINSEL4 &= ~(0x03 << 22);     // Clear P2.10 function
  LPC_PINCON->PINSEL4 |= (0x01 << 22);      // Set P2.10 to EINT0
  LPC_PINCON->PINMODE4 &= ~(0x03 << 22);    // Set P2.10 to pull-up mode
  LPC_PINCON->PINMODE_OD2 &= ~(0x01 << 11); // Set P2.10 to normal mode
  LPC_SC->EXTINT = 0x02;                    // Clear EINT0 interrupt flag
	NVIC_SetPriority(EINT1_IRQn, 31);
  NVIC_EnableIRQ(EINT1_IRQn);               // Enable EINT0 interrupt in NVIC
}

void EINT1_IRQHandler(void) {
  LPC_SC->EXTINT = 0x02; // Clear EINT0 interrupt flag
	for(uint32_t i = 0; i < 4; i++){
		LPC_GPIO0->FIOCLR = 0x02;
	  wait(500);
	  LPC_GPIO0->FIOSET = 0x02;
	  wait(500);
	  
	}
}

void GPIOINT_init(void) {
  LPC_GPIOINT->IO0IntEnF |=
      (0x01 << 19);           // Enable falling edge interrupt on P2.10
	NVIC_SetPriority(EINT3_IRQn, 1);
  NVIC_EnableIRQ(EINT3_IRQn); // Enable EINT3 interrupt in NVIC
}

void EINT3_IRQHandler(void) {
  LPC_GPIOINT->IO0IntClr = (0x01 << 19); // Clear P2.10 interrupt flag
  LPC_SC->EXTINT = (0x01 << 3);          // Clear EINT3 interrupt flag
  UART_write_string("Bang!\r\n");
}

void RTC_Clk_config(void) {
  LPC_PINCON->PINSEL3 &= ~(0x03 << 22);     // Clear P2.10 function
  LPC_PINCON->PINSEL3 |= (0x01 << 22);      // Set P2.10 to EINT0
	LPC_SC->CLKOUTCFG = 0x104;
}

int main() {

  SystemInit();
  SysTick_Config(SystemCoreClock / 1000);
  NVIC_SetPriority(SysTick_IRQn, 0);
	
  UART_init_reg();
  UART_write_string("LPC hello, UART connected!\r\n");

  Timer_init();
  RTC_init();
  EINT0_init();
  GPIOINT_init();
  EINT1_init();
	RTC_Clk_config();
  

  for (;;) {
	 /* __WFI();
  UART_write_string("LPC sleep wake up\r\n"); */
  }
}
