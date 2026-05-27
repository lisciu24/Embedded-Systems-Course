#ifndef UART_PARSER_H
#define UART_PARSER_H

#include "LPC17xx.h"
#include "generator_ctrl.h"
#include "uart.h"
#include "waves_lut.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
    Assumes that command from the UART is in format:
    WAVE_TYPE_NAME:FREQUENCY:AMPLITUDE\n
    where:
    WAVE_TYPE_NAME can be: SIN, SQUARE, TRIANGLE
    FREQUENCY is positive int in Hz (Hertz)
    AMPLITUDE is positive int in mV (miliVolts)
    \n is to mark the end of command
*/

typedef void (*UARTWaveFn)(uint16_t, uint16_t);

void gen_sin(uint16_t frequency, uint16_t amplitude) {
    // for debug
    UART_write_string("SIN:");
    char int_buf[64];
    int_to_str(frequency, int_buf, 10);
    UART_write_string(int_buf); // send frequency
    UART_write_string(":");
    int_to_str(amplitude, int_buf, 10);
    UART_write_string(int_buf); // send amplitude
    UART_write_string("\r\n");

    GENCTRL_function(sin_lut, amplitude, frequency);
}

void gen_square(uint16_t frequency, uint16_t amplitude) {
    // for debug
    UART_write_string("SQUARE:");
    char int_buf[64];
    int_to_str(frequency, int_buf, 10);
    UART_write_string(int_buf); // send frequency
    UART_write_string(":");
    int_to_str(amplitude, int_buf, 10);
    UART_write_string(int_buf); // send amplitude
    UART_write_string("\r\n");

    GENCTRL_function(square_lut, amplitude, frequency);
}

void gen_triangle(uint16_t frequency, uint16_t amplitude) {
    // for debug
    UART_write_string("TRIANGLE:");
    char int_buf[64];
    int_to_str(frequency, int_buf, 10);
    UART_write_string(int_buf); // send frequency
    UART_write_string(":");
    int_to_str(amplitude, int_buf, 10);
    UART_write_string(int_buf); // send amplitude
    UART_write_string("\r\n");

    GENCTRL_function(triangle_lut, amplitude, frequency);
}

typedef struct {
    const char *wave_name;
    UARTWaveFn wave_fn;
} UARTWaveMap;

static UARTWaveMap mapping_table[] = {
    {"SIN", gen_sin}, {"SQUARE", gen_square}, {"TRIANGLE", gen_triangle}};

#define MAPPING_COUNT (sizeof(mapping_table) / sizeof(mapping_table[0]))

#define UART_RX_BUF_SIZE 64

static volatile char uart_rx_buf[UART_RX_BUF_SIZE];

void parse_uart_command(char *command) {
    char *token;

    token = strtok(command, ":");

    if (token == NULL) // if command was empty string
    {
        return;
    }

    for (uint16_t i = 0; i < MAPPING_COUNT; i++) {
        if (strcmp(token, mapping_table[i].wave_name) == 0) {
            char *freq_str = strtok(NULL, ":");
            char *amp_str = strtok(NULL, ":");

            if (freq_str == NULL) {
                UART_write_string("Missing frequency\r\n");
                return;
            }

            if (amp_str == NULL) {
                UART_write_string("Missing amplitude\r\n");
                return;
            }
            uint16_t freq_int = atoi(freq_str);
            uint16_t amp_int = atoi(amp_str);

            if (freq_int == 0 ||
                amp_int == 0) // if the conversion is not valid atoi returns 0
            {
                UART_write_string("Frequency or amplitude is not int\r\n");
                return;
            }

            mapping_table[i].wave_fn(freq_int, amp_int);

            UART_write_string(mapping_table[i].wave_name);
            UART_write_string(" OK\r\n");
            return;
        }
    }

    UART_write_string("UNKNOWN WAVE NAME\r\n");
}

void UART0_IRQHandler(void) {
    UART_read_string(uart_rx_buf, UART_RX_BUF_SIZE);
    parse_uart_command(uart_rx_buf);
}

#endif // UART_PARSER_H