#include "uart.h"
#include "tokenizer.h"

#define UART_RX_BUF_SIZE 64

static volatile char uart_rx_buf[UART_RX_BUF_SIZE];
static volatile uint16_t uart_rx_idx = 0;

void int_to_str(int32_t num, char *str, uint32_t base) {
    uint32_t i = 0, neg = 0;

    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    if (num < 0) {
        neg = 1;
        num = -num;
    }

    while (num != 0) {
        uint8_t tmp = (num % base);
        str[i++] = (tmp < 10 ? tmp + '0' : tmp - 10 + 'A');
        num /= base;
    }

    if (neg) {
        str[i++] = '-';
    }

    str[i] = '\0';

    // Reverse the string
    for (int j = 0; j < i / 2; j++) {
        char temp = str[j];
        str[j] = str[i - j - 1];
        str[i - j - 1] = temp;
    }
}

void UART_write_byte(uint8_t data) {
    while (!(LPC_UART0->LSR & (1 << 5)))
        ;                  // Wait for THR to be empty
    LPC_UART0->THR = data; // Send data
}

void UART_write_string(const char *str) {
    while (*str) {
        UART_write_byte(*str++);
    }
}

void UART_write_line(const char *str) {
    UART_write_string(str);
    UART_write_string("\r\n");
}

void UART_write_int(uint32_t value) {
    char buff[16];
    int_to_str(value, buff, 10);
    UART_write_string(buff);
}

uint8_t UART_read_byte(void) {
    while (!(LPC_UART0->LSR & (1 << 0)))
        ;                  // Wait for data
    return LPC_UART0->RBR; // Read data
}

void UART_read_string(char buff[], uint32_t buff_size) {
    uint8_t c = UART_read_byte();
    UART_write_byte(c);
    uint32_t i = 0;
    while (c != '\n' && c != '\r' && i != buff_size - 1) {
        buff[i++] = c;
        c = UART_read_byte();
        UART_write_byte(c);
    }
    buff[i] = '\0';
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
    LPC_UART0->IER = 0x01; // Enable RBR interrupt
    LPC_UART0->FCR = 0x07; // Enable and reset TX/RX FIFO

    // Clear P0.2 and P0.3 function
    LPC_PINCON->PINSEL0 &= (~(0x03 << 4) & ~(0x03 << 6));
    // Set P0.2, P0.3 to TXD0, RXD0
    LPC_PINCON->PINSEL0 |= ((0x01 << 4) | (0x01 << 6));

    // Set P0.2, P0.3 to pull-up mode
    LPC_PINCON->PINMODE0 &= (~(0x03 << 4) & ~(0x03 << 6)); // optional
    // Set P0.2, P0.3 to normal mode
    LPC_PINCON->PINMODE_OD0 &= (~(0x01 << 2) & ~(0x01 << 3)); // optional

    NVIC_EnableIRQ(UART0_IRQn);
}

void UART0_IRQHandler(void) {
    uint32_t iir = LPC_UART0->IIR;
    if (iir & 1)
        return;
    uint8_t c = UART_read_byte();

    if (c == '\n' || c == '\r') // command from uart ends with \n
    {
        uart_rx_buf[uart_rx_idx] = '\0';
        uart_rx_idx = 0;

        UART_write_string("\r\nREAD: ");
        UART_write_string((char *)uart_rx_buf);
        UART_write_string("\r\n");
        CMD_parse((char *)uart_rx_buf);

    } else if (uart_rx_idx < UART_RX_BUF_SIZE - 1) {
        uart_rx_buf[uart_rx_idx++] = c;
    } else {
        UART_write_string("UART_RX_BUF_SIZE - exceeded size of rx buffer\r\n");
        uart_rx_idx = 0;
    }
}
