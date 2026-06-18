#include <stdio.h>
#define _TOK_TEST_

#include "../tokenizer.c"
#include "../waves_lut.c"

int main() {
    char buff[128];
    int idx = 0;
    while (1) {
        char c = fgetc(stdin);
        if (c == '\n' || c == '\r') {
            buff[idx] = '\0';
            idx = 0;

            CMD_parse((char *)buff);

        } else if (idx < 128 - 1) {
            buff[idx++] = c;
        } else {
            printf("UART_RX_BUF_SIZE - exceeded size of rx buffer\n");
            idx = 0;
        }
    }
}