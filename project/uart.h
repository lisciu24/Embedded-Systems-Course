#ifndef UART_H
#define UART_H

#include "LPC17xx.h"
#include "waves_lut.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void int_to_str(int32_t num, char *str, uint32_t base);

void UART_write_byte(uint8_t data);

void UART_write_string(const char *str);

uint8_t UART_read_byte(void);

void UART_read_string(char buff[], uint32_t buff_size);

void UART_init_reg(void);

/*
    Assumes that command from the UART is in format:
    WAVE_TYPE_NAME:FREQUENCY:AMPLITUDE\n
    where:
    WAVE_TYPE_NAME can be: SIN, SQUARE, TRIANGLE
    FREQUENCY is positive int in Hz (Hertz)
    AMPLITUDE is positive int in mV (miliVolts)
    \n is to mark the end of command
*/

void gen_sin(uint16_t frequency, uint16_t amplitude);
void gen_square(uint16_t frequency, uint16_t amplitude);
void gen_triangle(uint16_t frequency, uint16_t amplitude);
void parse_uart_command(char *command);
void UART0_IRQHandler(void);

#endif // UART_H
