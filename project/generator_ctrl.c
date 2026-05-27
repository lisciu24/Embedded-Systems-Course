#include "generator_ctrl.h"
#include "LPC17xx.h"
#include "uart.h"

// DAC has 10 bit resolution so value range [0,1023]
// DACR has DAC value on [15:6]
#define DACV(x) ((x) << 6)

#define CLIP_FREQUENCY(freq)                                                   \
    ((freq) > MAX_FREQUENCY                                                    \
         ? MAX_FREQUENCY                                                       \
         : ((freq) < MIN_FREQUENCY ? MIN_FREQUENCY : (freq)))
#define CLIP_AMPLITUDE(amp) ((amp) > MAX_AMPLITUDE ? MAX_AMPLITUDE : (amp))

typedef struct LLI_s {
    uint32_t DMACCSrcAddr;
    uint32_t DMACCDestAddr;
    struct LLI_s *DMACCLLI;
    uint32_t DMACCControl;
} LLI_t;

LLI_t LLI;

static uint16_t buff[FSAMPLE];

void LLI_init(void) {
    LLI.DMACCLLI = &LLI;
    // Set source address for LLI
    LLI.DMACCSrcAddr = (uint32_t)buff;
    // Set destination address for LLI
    LLI.DMACCDestAddr = (uint32_t)&LPC_DAC->DACR;
    LLI.DMACCControl = FSAMPLE | 1 << 18 | 1 << 21 | 1 << 26; // | 1U << 31;
}

void DMA_init(void) {
    LPC_SC->PCONP |= 1 << 29;            // Power up DMA
    LPC_GPDMA->DMACConfig = 0b01;        // Enable DMA controller
    LPC_GPDMA->DMACIntErrClr |= 1 << 0;  // Clear error interrupt
    LPC_GPDMA->DMACIntTCClear |= 1 << 0; // Clear transcation finished interrupt
    // Set source address for chanel 0
    LPC_GPDMACH0->DMACCSrcAddr = (uint32_t)buff;
    // Set destination address for chanel 0
    LPC_GPDMACH0->DMACCDestAddr = (uint32_t)&LPC_DAC->DACR;
    // Set LLI address
    LPC_GPDMACH0->DMACCLLI = (uint32_t)&LLI;

    // Sets: TransferSize to values_size,
    // SWidth to 16-bit, DWidth to 16-bit,
    // Source increment
    LPC_GPDMACH0->DMACCControl =
        FSAMPLE | 1 << 18 | 1 << 21 | 1 << 26 | 1U << 31;

    // Enables DMA channel,
    // Sets: DestPeripheral to DAC,
    // TransferType memory to peripheral
    LPC_GPDMACH0->DMACCConfig = 1 | 0x07 << 6 | 0b001 << 11 | 0b11 << 14;
    NVIC_EnableIRQ(DMA_IRQn);
}

void DMA_IRQHandler(void) {
    LPC_GPDMA->DMACIntErrClr |= 1 << 0;  // Clear error interrupt
    LPC_GPDMA->DMACIntTCClear |= 1 << 0; // Clear transcation finished interrupt
    UART_write_string("DMA\n\r");
}

void DMA_stop(void) {
    LPC_GPDMACH0->DMACCConfig |= 1 << 18;
    while (LPC_GPDMACH0->DMACCConfig & 1 << 17)
        ;
    LPC_GPDMACH0->DMACCConfig &= ~1;
}

void DAC_init(void) {
    // Set P0.26 to AOUT function, enables DAC
    LPC_PINCON->PINSEL1 &= ~(0b11 << 20);
    LPC_PINCON->PINSEL1 |= 0b10 << 20;

    // Enable DMA, double buffering, time-out counter
    // TODO why bit DMA_ENA is LOW???
    LPC_DAC->DACCTRL = 0b0110;

    // pclk 25MHz, max update rate 1MHz
    // 16-bit timer, allowed values 25 and bigger
    LPC_DAC->DACCNTVAL = 100U;
}

void DAC_set_frequency(uint32_t freq) {
    freq = CLIP_FREQUENCY(freq);
    LPC_DAC->DACCNTVAL = 25000000 / freq / FSAMPLE;
}

void GENCTRL_init(void) {
    DAC_init();
    LLI_init();
}

void GENCTRL_function(const uint16_t fun[], uint32_t amplitude,
                      uint32_t frequency) {
    GENCTRL_stop();
    DAC_set_frequency(frequency);
    for (uint32_t i = 0; i < FSAMPLE; i++) {
        uint16_t scaled = fun[i] * CLIP_AMPLITUDE(amplitude) / 3300;
        buff[i] = DACV(scaled);
    }
    GENCTRL_start();
}

void GENCTRL_start() { DMA_init(); }

void GENCTRL_stop() { DMA_stop(); }