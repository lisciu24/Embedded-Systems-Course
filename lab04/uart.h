#ifndef UART_H
#define UART_H

#include "LPC17xx.h"

void UART_write_byte(uint8_t data) {
  while (!(LPC_UART0->LSR & (1 << 5)))
    ;                    // Wait for THR to be empty
  LPC_UART0->THR = data; // Send data
}

void UART_write_string(const char *str) {
  while (*str) {
    UART_write_byte(*str++);
  }
}

uint8_t UART_read_byte(void) {
  while (!(LPC_UART0->LSR & (1 << 0)))
    ;                    // Wait for data
  return LPC_UART0->RBR; // Read data
}

void UART_init_reg(void) {
  LPC_SC->PCONP |= (1 << 3); // Power up UART0

  LPC_SC->PCLKSEL0 &= ~(0x03 << 6); // Clear PCLK_UART0
  LPC_SC->PCLKSEL0 |= (0x00 << 6);  // Set PCLK_UART0

  LPC_UART0->LCR = (1 << 7); // Enable DLAB
  // baud rate 115200 with 25 MHz PCLK
  LPC_UART0->DLM = 0x00;               // Set baud rate high
  LPC_UART0->DLL = 0x09;               // Set baud rate low
  LPC_UART0->FDR = (0x02 << 4) | 0x01; // MULVAL | DIVADDVAL
  LPC_UART0->LCR = 0x03; // 8 bits, no parity, 1 stop bit, disable DLAB
  LPC_UART0->FCR = 0x07; // Enable and reset TX/RX FIFO

  // Clear P0.2 and P0.3 function
  LPC_PINCON->PINSEL0 &= (~(0x03 << 4) & ~(0x03 << 6));
  // Set P0.2, P0.3 to TXD0, RXD0
  LPC_PINCON->PINSEL0 |= ((0x01 << 4) | (0x01 << 6));

  // Set P0.2, P0.3 to pull-up mode
  LPC_PINCON->PINMODE0 &= (~(0x03 << 4) & ~(0x03 << 6)); // optional
  // Set P0.2, P0.3 to normal mode
  LPC_PINCON->PINMODE_OD0 &= (~(0x01 << 2) & ~(0x01 << 3)); // optional
}

#endif
