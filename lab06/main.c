#include "LPC17xx.h"
#include "uart.h"
#include <stdint.h>

const uint8_t diodes[] = {~0x11, ~0x23, ~0x47, ~0x8F};
uint32_t diodes_size = sizeof(diodes) / sizeof(diodes[0]);
volatile uint32_t diodes_idx = 0;

typedef struct LLI_s {
  uint32_t DMACCSrcAddr;
  uint32_t DMACCDestAddr;
  struct LLI_s *DMACCLLI;
  uint32_t DMACCControl;
} LLI_t;

LLI_t LLI;

void init_LEDs(void) {
  LPC_GPIO2->FIODIR0 = 0xFF; // Set LEDs pins to output
  LPC_GPIO2->FIOSET0 = 0xFF; // Set LEDs pins high (turn them off)
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
  UART_write_string("Click!\r\n");
}

void init_Timer1(void) {
  LPC_SC->PCONP |= 1 << 2;          // Power up Timer1
  LPC_SC->PCLKSEL0 &= ~(0b11 << 4); // Select PCLK = CCLK/4
  LPC_TIM1->PR = 1e6 - 1;      // Set prescaler to 10^6
  LPC_TIM1->MR0 = 50;          // Set match 0 to 50
  LPC_TIM1->MCR = 0x03;        // Set interrupt and reset on match 0
  LPC_TIM1->IR = 0x3F;         // Clear interrupts
  LPC_TIM1->TCR = 0x01;        // Start timer
  NVIC_EnableIRQ(TIMER1_IRQn); // Enable Timer1 interrupt in NVIC
}


/// TODO comment that function before enabling DMA
void TIMER1_IRQHandler(void) {
	LPC_TIM1->IR = 0x3F; // Clear interrupts

  //uint32_t next_idx = (diodes_idx + 1) % diodes_size; // Calc new diodes index
  //LPC_GPIO2->FIOPIN0 = diodes[next_idx]; // Switch diodes to new pattern
  //diodes_idx = next_idx;      	// Set new index
	UART_write_string("Timer IRQ\n\r");
}

void init_LLI(void) {
  LLI.DMACCLLI = &LLI;
  LLI.DMACCSrcAddr = (uint32_t)diodes; // Set source address for LLI
  LLI.DMACCDestAddr =
      (uint32_t)&LPC_GPIO2->FIOPIN0; // Set destination address for LLI
  LLI.DMACCControl = (diodes_size | 1 << 26);  
}

void init_DMA(void) {
  LPC_SC->PCONP |= 1 << 29;     // Powet up DMA
  LPC_GPDMA->DMACConfig = 0b01; // Enable DMA controller
  LPC_SC->DMAREQSEL |= 1 << 2;  // Set Timer 1 match 0 as input on channel 15
  LPC_GPDMA->DMACIntErrClr |= 1 << 0;  // Clear error interrupt
  LPC_GPDMA->DMACIntTCClear |= 1 << 0; // Clear transcation finished interrupt
  LPC_GPDMACH0->DMACCSrcAddr =
      (uint32_t)diodes; // Set source address for chanel 0
  LPC_GPDMACH0->DMACCDestAddr =
      (uint32_t)&LPC_GPIO2->FIOPIN0; // Set destination address for chanel 0
  /// TODO change LLI to created structure after testing with handler
  LPC_GPDMACH0->DMACCLLI = (uint32_t) &LLI; // Indicate that this is last, disable DMA after transfer completion
  // Clearing conrtrol register sets source and destination size to 8 bits,
  // burst size is set to 1
	// Set transfer size -  how many elements will be transfered
	// Turn on source address increment
	// Enable interrupt on transfer completion
  LPC_GPDMACH0->DMACCControl = (diodes_size | 1 << 26);    
	// Enable DMA channel 0	
 // Set destination peripherial to Timer 1 match 0	
	 // Set transfer type to memory to peripherial
	// Disable mask for error and terminal interrupt

  LPC_GPDMACH0->DMACCConfig = 1 | 0x0A << 6 | 0b001 << 11 | 0b11 << 14; 
  NVIC_EnableIRQ(DMA_IRQn);       // Enable DMA interrupt in NVIC
}

void DMA_IRQHandler(void) {
    LPC_GPDMA->DMACIntErrClr |= 1 << 0;  // Clear error interrupt
  LPC_GPDMA->DMACIntTCClear |= 1 << 0; // Clear transcation finished interrupt
  LPC_GPDMACH0->DMACCSrcAddr =
      (uint32_t)diodes; // Set source address for chanel 0
  LPC_GPDMACH0->DMACCDestAddr =
      (uint32_t)&LPC_GPIO2->FIOPIN0; // Set destination address for chanel 0
  /// TODO change LLI to created structure after testing with handler
  LPC_GPDMACH0->DMACCLLI =
      0; // Indicate that this is last, disable DMA after transfer completion
  // Clearing conrtrol register sets source and destination size to 8 bits,
  // burst size is set to 1
	// Set transfer size -  how many elements will be transfered
	// Turn on source address increment
	// Enable interrupt on transfer completion
  LPC_GPDMACH0->DMACCControl = (diodes_size | 1 << 26 | 1U << 31);    
	// Enable DMA channel 0	
 // Set destination peripherial to Timer 1 match 0	
	 // Set transfer type to memory to peripherial
	// Disable mask for error and terminal interrupt

  LPC_GPDMACH0->DMACCConfig = 1 | 0x0A << 6 | 0b001 << 11 | 0b11 << 14; 
	UART_write_string("DMA IRQ\r\n");
}

int main() {
  UART_init_reg();
  UART_write_string("UART LPC");
  init_LEDs();
  EINT0_init();
  init_LLI();
  init_DMA();
  init_Timer1();

  for (;;) {
    UART_write_string("ide spac");
    __WFI(); // put processor to sleep
	      UART_write_string("pobudka");

  }
}
