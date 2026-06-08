#ifndef UART_H
#define UART_H

#include "LPC17xx.h"

void UART_write_byte(uint8_t data);
void UART_write_string(const char *str);
void UART_write_line(const char *str);
void UART_write_int(uint32_t value);

uint8_t UART_read_byte(void);
void UART_read_string(char buff[], uint32_t buff_size);

void UART_init_reg(void);
void UART0_IRQHandler(void);

#endif // UART_H
