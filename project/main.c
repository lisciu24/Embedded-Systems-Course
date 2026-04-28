#include "LPC17xx.h"
#include "uart.h"

#define DACV(x) (x << 6)

uint16_t values[] = {DACV(0x0000), DACV(0x01FF), DACV(0x03FF)};
uint32_t values_size = sizeof(values) / sizeof(values[0]);

typedef struct LLI_s {
  uint32_t DMACCSrcAddr;
  uint32_t DMACCDestAddr;
  struct LLI_s *DMACCLLI;
  uint32_t DMACCControl;
} LLI_t;

LLI_t LLI;

void LLI_init(void) {
  LLI.DMACCLLI = &LLI;
  // Set source address for LLI
  LLI.DMACCSrcAddr = (uint32_t)values;
  // Set destination address for LLI
  LLI.DMACCDestAddr = (uint32_t)&LPC_DAC->DACR;
  LLI.DMACCControl = values_size | 1 << 18 | 1 << 21 | 1 << 26;
}

void DAC_init(void) {
  // Set P0.26 to AOUT function, enables DAC
  LPC_PINCON->PINSEL1 &= ~(0b11 << 20);
  LPC_PINCON->PINSEL1 |= 0b10 << 20;

  // Enable DMA, double buffering, time-out counter
  LPC_DAC->DACCTRL = 0b1110;

  // pclk 25MHz, max update rate 1MHz
  // 16-bit timer, allowed values 25 and bigger
  LPC_DAC->DACCNTVAL = 50U;
}

void DMA_init(void) {
  LPC_SC->PCONP |= 1 << 29;            // Power up DMA
  LPC_GPDMA->DMACConfig = 0b01;        // Enable DMA controller
  LPC_GPDMA->DMACIntErrClr |= 1 << 0;  // Clear error interrupt
  LPC_GPDMA->DMACIntTCClear |= 1 << 0; // Clear transcation finished interrupt
  // Set source address for chanel 0
  LPC_GPDMACH0->DMACCSrcAddr = (uint32_t)values;
  // Set destination address for chanel 0
  LPC_GPDMACH0->DMACCDestAddr = (uint32_t)&LPC_DAC->DACR;
  // Set LLI address
  LPC_GPDMACH0->DMACCLLI = (uint32_t)&LLI;

  // Sets: TransferSize to values_size,
  // SWidth to 16-bit, DWidth to 16-bit,
  // Source increment
  LPC_GPDMACH0->DMACCControl = values_size | 1 << 18 | 1 << 21 | 1 << 26;

  // Enables DMA channel,
  // Sets: DestPeripheral to DAC,
  // TransferType memory to peripheral
  LPC_GPDMACH0->DMACCConfig = 1 | 0x04 << 6 | 0b001 << 11;
}

int main() {

  LLI_init();
  DAC_init();
  DMA_init();

  for (;;) {
  }
}
