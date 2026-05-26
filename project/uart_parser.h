#ifndef UART_PARSER_H
#define UART_PARSER_H

#include "LPC17xx.h"
#include <stdbool.h>
#include "uart.h"
#include <string.h>
#include <stdlib.h>

/*
    Assumes that command from the UART is in format:
    WAVE_TYPE_NAME:FREQUENCY:AMPLITUDE\n
    where:
    WAVE_TYPE_NAME can be: SIN, SQUARE, TRIANGLE
    FREQUENCY is positive int in Hz (Hertz)
    AMPLITUDE is positive int in mV (miliVolts)
    \n is to mark the end of command
*/

typedef void (*UARTWaveFn)(void);


void sin(uint16_t frequency, uint16_t amplitude)
{
    // for debug
    UART_write_string("SIN:");
    char int_buf[64];
    int_to_str(frequency, int_buf, 10);
    UART_write_string(int_buf);//send frequency
    UART_write_string(":");
    int_to_str(amplitude, int_buf, 10);
    UART_write_string(int_buf); //send amplitude
    UART_write_string("\r\n");

    // TODO: start sending sin bytes from DMA?
}


void square(uint16_t frequency, uint16_t amplitude)
{
    // for debug
    UART_write_string("SQUARE:");
    char int_buf[64];
    int_to_str(frequency, int_buf, 10);
    UART_write_string(int_buf);//send frequency
    UART_write_string(":");
    int_to_str(amplitude, int_buf, 10);
    UART_write_string(int_buf); //send amplitude
    UART_write_string("\r\n");

    // TODO: start sending square bytes from DMA?
}


void triangle(uint16_t frequency, uint16_t amplitude)
{
    // for debug
    UART_write_string("TRIANGLE:");
    char int_buf[64];
    int_to_str(frequency, int_buf, 10);
    UART_write_string(int_buf);//send frequency
    UART_write_string(":");
    int_to_str(amplitude, int_buf, 10);
    UART_write_string(int_buf); //send amplitude
    UART_write_string("\r\n");

    // TODO: start sending triangle bytes from DMA?
}

typedef struct
{
    const char *wave_name;
    UARTWaveFn wave_fn;
} UARTWaveMap;

static UARTWaveMap mapping_table[] =
{
    {"SIN", sin},
    {"SQUARE", square},
    {"TRIANGLE", triangle}
};

#define MAPPING_COUNT (sizeof(mapping_table) / sizeof(mapping_table[0]))

#define UART_RX_BUF_SIZE 64

static volatile char uart_rx_buf[UART_RX_BUF_SIZE];
static volatile uint16_t uart_rx_idx = 0;
static volatile bool uart_rx_buf_ready = false;


void parse_uart_command(char *command)
{
    char *token;

    token = strtok(command, ":");

    if (token == NULL) //if command was empty string
    {
        return;
    }

    for(uint16_t i = 0; i < MAPPING_COUNT; i++)
    {
        if (strcmp(token, mapping_table[i].wave_name) == 0)
        {
            char *freq_str = strtok(NULL, ":");
            char *amp_str  = strtok(NULL, ":");

            if (freq_str == NULL)
            {
                UART_write_string("Missing frequency\r\n");
                return;
            }

            if (amp_str == NULL)
            {
                UART_write_string("Missing amplitude\r\n");
                return;
            }
            uint16_t freq_int = atoi(freq_str);
            uint16_t amp_int = atoi(amp_str);

            if (freq_int == 0 || amp_int == 0) // if the conversion is not valid atoi returns 0
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


void UART0_IRQHandler(void)
{
    uint8_t c = UART_read_byte();

    if (uart_rx_buf_ready)
    {
        uart_rx_buf_ready = false;
        parse_uart_command((char*)uart_rx_buf);
    }

    if (c == '\n') // command from uart ends with \n
    {
        uart_rx_buf[uart_rx_idx] = '\0';
        uart_rx_buf_ready = true;
        uart_rx_idx = 0;
    }
    else
    {
        if (uart_rx_idx < UART_RX_BUF_SIZE - 1)
        {
            uart_rx_buf[uart_rx_idx++] = c;
        }
        else
        {
            UART_write_string("UART_RX_BUF_SIZE - exceeded size of rx buffer\r\n");
            uart_rx_idx = 0;
        }
    }
}

#endif //UART_PARSER_H