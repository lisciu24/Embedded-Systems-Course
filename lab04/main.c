#include "LPC17xx.h"
#include "uart.h"

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
  NVIC_EnableIRQ(RTC_IRQn);  // Enable RTC interrupt in NVIC
}

void RTC_IRQHandler(void) {
  LPC_RTC->ILR = 0x01;  // Clear second interrupt flag
  RTC_flag = ~RTC_flag; // Toggle flag
  UART_write_string(RTC_flag ? "Tik!\r\n" : "Tak!\r\n");
}

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

void EINT0_IRQHandler(void) {
  LPC_SC->EXTINT = 0x01; // Clear EINT0 interrupt flag
  /// TODO toggle LED here
  UART_write_string("Click!\r\n");
}

void GPIOINT_init(void) {
  LPC_GPIOINT->IO0IntEnF |=
      (0x01 << 19);           // Enable falling edge interrupt on P2.10
  NVIC_EnableIRQ(EINT3_IRQn); // Enable EINT3 interrupt in NVIC
}

void EINT3_IRQHandler(void) {
  LPC_GPIOINT->IO0IntClr = (0x01 << 19); // Clear P2.10 interrupt flag
  LPC_SC->EXTINT = (0x01 << 3);          // Clear EINT3 interrupt flag
  UART_write_string("Bang!\r\n");
}

int main() {

  SystemInit();
  UART_init_reg();
  UART_write_string("LPC hello, UART connected!\r\n");

  Timer_init();
  RTC_init();
  EINT0_init();
  //   GPIOINT_init();

  for (;;) {
  }
}
