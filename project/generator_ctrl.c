#include "generator_ctrl.h"
#include "LPC17xx.h"
#include "uart.h"
#include <stdint.h>

// DAC has 10 bit resolution so value range [0,1023]
// DACR has DAC value on [15:6]
#define DACV(x) ((x) << 6)

#define CLIP_FREQUENCY(freq)                                                   \
    ((freq) > MAX_FREQUENCY                                                    \
         ? MAX_FREQUENCY                                                       \
         : ((freq) < MIN_FREQUENCY ? MIN_FREQUENCY : (freq)))
#define CLIP_AMPLITUDE(amp) ((amp) > MAX_AMPLITUDE ? MAX_AMPLITUDE : (amp))

#define PIXEL_HEIGHT (512 / BITMAP_SIZE)
#define BITMAP_ROW_BUFF (1 + BITMAP_SIZE * PIXEL_RES + 1)

typedef struct LLI_s {
    void* DMACCSrcAddr;
    void* DMACCDestAddr;
    const struct LLI_s *DMACCLLI;
    uint32_t DMACCControl;
} LLI_t;

static uint16_t bitmap[BITMAP_SIZE][BITMAP_SIZE] = {
    {0, 0, 1, 1, 1, 1, 0, 0}, 
{0, 1, 0, 0, 0, 0, 1, 0},
{1, 0, 0, 1, 1, 0, 0, 1},

    {1, 0, 0, 0, 0, 0, 0, 1},
 {1, 0, 1, 0, 0, 1, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1}, 
    {0, 1, 0, 0, 0, 0, 1, 0},
{0, 0, 1, 1, 1, 1, 0, 0}};

static uint16_t bmp_buff[BITMAP_ROW_BUFF * 2];
static const LLI_t LLI_bitmap[2] = {
    {.DMACCLLI = &LLI_bitmap[1],
     // Set source address for LLI
     .DMACCSrcAddr = bmp_buff,
     // Set destination address for LLI
     .DMACCDestAddr = (void*)&LPC_DAC->DACR,
     .DMACCControl = BITMAP_ROW_BUFF | 1 << 18 | 1 << 21 | 1 << 26 | 1U << 31},
    {.DMACCLLI = &LLI_bitmap[0],
     // Set source address for LLI
     .DMACCSrcAddr = (bmp_buff + BITMAP_ROW_BUFF),
     // Set destination address for LLI
     .DMACCDestAddr = (void*)&LPC_DAC->DACR,
     .DMACCControl = BITMAP_ROW_BUFF | 1 << 18 | 1 << 21 | 1 << 26 | 1U << 31}};
static volatile uint32_t bmp_idx = 0;
static volatile uint32_t bmp_row_idx = 0;

static uint16_t buff[FSAMPLE];
static const LLI_t LLI = {
    .DMACCLLI = &LLI,
    // Set source address for LLI
    .DMACCSrcAddr = buff,
    // Set destination address for LLI
    .DMACCDestAddr = (void*)&LPC_DAC->DACR,
    .DMACCControl = FSAMPLE | 1 << 18 | 1 << 21 | 1 << 26 // | 1U << 31;
};

static void DMA_init(void) {
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
        FSAMPLE | 1 << 18 | 1 << 21 | 1 << 26; //| 1U << 31;

    // Enables DMA channel,
    // Sets: DestPeripheral to DAC,
    // TransferType memory to peripheral
    LPC_GPDMACH0->DMACCConfig = 1 | 0x07 << 6 | 0b001 << 11 | 0b11 << 14;
    NVIC_EnableIRQ(DMA_IRQn);
}

static void DMA_bitmap_init() {
    LPC_SC->PCONP |= 1 << 29;            // Power up DMA
    LPC_GPDMA->DMACConfig = 0b01;        // Enable DMA controller
    LPC_GPDMA->DMACIntErrClr |= 1 << 0;  // Clear error interrupt
    LPC_GPDMA->DMACIntTCClear |= 1 << 0; // Clear transcation finished interrupt
    // Set source address for chanel 0
    LPC_GPDMACH0->DMACCSrcAddr = (uint32_t)bmp_buff;
    // Set destination address for chanel 0
    LPC_GPDMACH0->DMACCDestAddr = (uint32_t)&LPC_DAC->DACR;
    // Set LLI address
    LPC_GPDMACH0->DMACCLLI = (uint32_t)&LLI_bitmap[1];

    // Sets: TransferSize to values_size,
    // SWidth to 16-bit, DWidth to 16-bit,
    // Source increment
    LPC_GPDMACH0->DMACCControl =
        BITMAP_ROW_BUFF | 1 << 18 | 1 << 21 | 1 << 26 | 1U << 31;

    // Enables DMA channel,
    // Sets: DestPeripheral to DAC,
    // TransferType memory to peripheral
    LPC_GPDMACH0->DMACCConfig = 1 | 0x07 << 6 | 0b001 << 11 | 0b11 << 14;
    NVIC_EnableIRQ(DMA_IRQn);
}

void DMA_IRQHandler(void) {
    LPC_GPDMA->DMACIntErrClr |= 1 << 0;  // Clear error interrupt
    LPC_GPDMA->DMACIntTCClear |= 1 << 0; // Clear transcation finished interrupt
    GENCTRL_bitmap_row(bmp_idx, bmp_row_idx);
    bmp_idx ^= 1;
    bmp_row_idx = (bmp_row_idx + 1) % BITMAP_SIZE;
}

static void DMA_stop(void) {
    LPC_GPDMACH0->DMACCConfig |= 1 << 18;
    while (LPC_GPDMACH0->DMACCConfig & 1 << 17)
        ;
    LPC_GPDMACH0->DMACCConfig &= ~1;
}

static void DAC_init(void) {
    // Set P0.26 to AOUT function, enables DAC
    LPC_PINCON->PINSEL1 &= ~(0b11 << 20);
    LPC_PINCON->PINSEL1 |= 0b10 << 20;

    // Enable DMA, double buffering, time-out counter
    // TODO why bit DMA_ENA is LOW???
    LPC_DAC->DACCTRL = 0b1110;

    // pclk 25MHz, max update rate 1MHz
    // 16-bit timer, allowed values 25 and bigger
    LPC_DAC->DACCNTVAL = 100U;
}

static void DAC_set_frequency(uint32_t freq) {
    freq = CLIP_FREQUENCY(freq);
    LPC_DAC->DACCNTVAL = 25000000 / freq / FSAMPLE;
}

static void DAC_set_bmp_row_frequency(uint32_t freq) {
    LPC_DAC->DACCNTVAL = 25000000 / freq / BITMAP_ROW_BUFF;
}

void GENCTRL_init(void) { DAC_init(); }

void GENCTRL_function(const uint16_t fun[], uint32_t amplitude,
                      uint32_t frequency) {
    GENCTRL_stop();
    DAC_set_frequency(frequency);
    for (uint32_t i = 0; i < FSAMPLE; i++) {
        uint16_t scaled = fun[i] * CLIP_AMPLITUDE(amplitude) / MAX_AMPLITUDE;
        buff[i] = DACV(scaled);
    }
    GENCTRL_start();
}

void GENCTRL_bitmap() {
    GENCTRL_stop();
    DAC_set_bmp_row_frequency(100);
    GENCTRL_bitmap_row(0, 0);
    GENCTRL_bitmap_row(1, 1);
    bmp_idx = 0;
    bmp_row_idx = 2;
    DMA_bitmap_init();
}

void GENCTRL_load_bitmap_row(uint32_t bmp_row, uint32_t bmp_row_idx) {
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        uint32_t mask = 1 << i;
        bitmap[BITMAP_SIZE - bmp_row_idx - 1][BITMAP_SIZE - i - 1] = (mask & bmp_row) >> i;
    }
}

void GENCTRL_bitmap_row(uint32_t bmp_buff_idx, uint32_t bmp_row_idx) {
    uint16_t *p_bmp_buff = bmp_buff + (BITMAP_ROW_BUFF * bmp_buff_idx);
    const uint16_t *p_bmp_row = bitmap[bmp_row_idx];
    const uint16_t base_offset = bmp_row_idx * PIXEL_HEIGHT;
    *p_bmp_buff++ = DACV(1023);
    for (uint32_t i = 0; i < BITMAP_SIZE; i++) {
        if (p_bmp_row[i] == 1) {
            for (uint32_t j = 0; j < PIXEL_RES; j += 2) {
                *p_bmp_buff++ = DACV(base_offset + PIXEL_HEIGHT - 1);
                *p_bmp_buff++ = DACV(base_offset);
            }
        } else {
            for (uint32_t j = 0; j < PIXEL_RES; j++) {
                *p_bmp_buff++ = DACV(base_offset);
            }
        }
    }
    *p_bmp_buff = 0;
}

void GENCTRL_start() { DMA_init(); }

void GENCTRL_stop() { DMA_stop(); }
