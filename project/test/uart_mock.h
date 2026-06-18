#ifndef UART_MOCK_H
#define UART_MOCK_H

#include <stdint.h>
#include <stdio.h>

static inline void int_to_str(int32_t num, char *str, uint32_t base) {
    (void)base;
    snprintf(str, 12, "%d", num);
}

static inline void UART_write_byte(uint8_t data) { fputc((int)data, stdout); }

static inline void UART_write_string(const char *str) { fputs(str, stdout); }

static inline void UART_write_line(const char *str) {
    fputs(str, stdout);
    fputc('\n', stdout);
}

static inline void UART_debug(const char *str) {
    fputs("DEBUG >>> ", stdout);
    UART_write_line(str);
}

static inline void UART_write_int(uint32_t value) { printf("%u", value); }

static inline uint8_t UART_read_byte(void) { return 0; }

static inline void UART_read_string(char buff[], uint32_t buff_size) {
    (void)buff;
    (void)buff_size;
}

static inline void UART_init_reg(void) {}

#endif // UART_MOCK_H