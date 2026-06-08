#include "tokenizer.h"
#include "generator_ctrl.h"
#include "uart.h"
#include "waves_lut.h"
#include <ctype.h>
#include <stdint.h>
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

typedef enum { TOKEN_STRING, TOKEN_UINT, TOKEN_END } token_type_t;

typedef struct {
    token_type_t type;
    const char *start; // Pointer to first character
    uint32_t length;   // length of token
} token_t;

typedef enum { ARG_STRING, ARG_UINT } arg_type_t;

typedef struct {
    arg_type_t type;
    union {
        uint32_t num;
        token_t str;
    } val;
} arg_t;

typedef enum { CMD_GEN_WAVE, CMD_FUN } cmd_type_t;

typedef void (*cmd_fun_t)(uint32_t, arg_t[]);

typedef struct {
    const char *name;
    cmd_type_t cmd_type;

    union {
        const uint16_t *wave_lut;
        cmd_fun_t fun;
    };
} cmd_t;

static void msg_help() {
    UART_write_line("Allowed commands:");
    UART_write_line("HELP  -> shows this help message");
    UART_write_line("START -> start generator with previously selected wave, "
                    "(default SIN)");
    UART_write_line("STOP  -> stops generator");
    UART_write_line("<WAVE>:<FREQ>:<AMP> -> start generator with <WAVE> wave, "
                    "frequency <FREQ> and amplitude <AMP>");
    UART_write_line("Valid <WAVE> values are: SIN, SQUARE, TRIANGLE");
    UART_write_line("Valid <FREQ> values are integers in range [10, 20 000]");
    UART_write_line("Valid <AMP> values are integers in range [100, 3300]");
}

static void cmd_help(uint32_t argc, arg_t argv[]) { msg_help(); }
static void cmd_start(uint32_t argc, arg_t argv[]) { GENCTRL_start(); }
static void cmd_stop(uint32_t argc, arg_t argv[]) { GENCTRL_stop(); }

static const cmd_t cmds[] = {
    {"SIN", CMD_GEN_WAVE, .wave_lut = sin_lut},
    {"SQUARE", CMD_GEN_WAVE, .wave_lut = square_lut},
    {"TRIANGLE", CMD_GEN_WAVE, .wave_lut = triangle_lut},
    {"HELP", CMD_FUN, .fun = cmd_help},
    {"START", CMD_FUN, .fun = cmd_start},
    {"STOP", CMD_FUN, .fun = cmd_stop}};

#define CMD_COUNT (sizeof(cmds) / sizeof(cmd_t))
#define MAX_ARG_COUNT 2

typedef enum {
    STATE_WHITESPACE,
    STATE_READ_WORD,
    STATE_READ_UINT
} lexer_state_t;

static uint32_t token_match(const token_t *token, const char *str) {
    if (token->type != TOKEN_STRING)
        return 0;
    return strncmp(token->start, str, token->length) == 0;
}

static uint32_t token_to_uint32(const token_t *token, uint32_t *value) {
    if (token->type != TOKEN_UINT)
        return 0;

    const char *cursor = token->start;
    for (uint32_t i = 0; i < token->length; i++, cursor++) {
        if (isdigit(*cursor)) {
            *value = *value * 10 + (*cursor - '0');
        } else {
            return 0;
        }
    }

    return 1;
}

static const char *get_next_token(const char *cursor, token_t *token) {
    lexer_state_t state = STATE_WHITESPACE;
    token->start = NULL;
    token->length = 0;

    while (*cursor != '\0' && *cursor != ':') {
        const char ch = *cursor;

        switch (state) {
        case STATE_WHITESPACE:
            if (isspace(ch)) {
                cursor++;
            } else if (isdigit(ch)) {
                state = STATE_READ_UINT;
                token->type = TOKEN_UINT;
                token->start = cursor;
                cursor++;
            } else if (isalpha(ch)) {
                state = STATE_READ_WORD;
                token->type = TOKEN_STRING;
                token->start = cursor;
                cursor++;
            }
            break;

        case STATE_READ_WORD:
            if (!isalpha(ch)) {
                token->length = cursor - token->start;
                return cursor; // end of word
            }
            cursor++;
            break;

        case STATE_READ_UINT:
            if (!isdigit(ch)) {
                token->length = cursor - token->start;
                return cursor; // end of uint
            }
            cursor++;
            break;
        }
    }

    if (state != STATE_WHITESPACE) {
        token->length = cursor - token->start;
        return cursor;
    }

    token->type = TOKEN_END;
    return cursor;
}

void CMD_parse(const char *command) {
    const char *cursor = command;
    token_t curr_token;

    cursor = get_next_token(cursor, &curr_token);
    if (curr_token.type != TOKEN_STRING) {
        UART_write_line("Error parsing command!");
        msg_help();
        return;
    }

    token_t cmd_token = curr_token;

    uint32_t argc = 0;
    arg_t argv[MAX_ARG_COUNT];

    for (argc = 0; argc < MAX_ARG_COUNT; argc++) {
        cursor = get_next_token(cursor + 1, &curr_token);
        if (curr_token.type == TOKEN_END)
            break;

        if (curr_token.type == TOKEN_STRING) {
            argv[argc].type = ARG_STRING;
            argv[argc].val.str = curr_token;
        } else if (curr_token.type == TOKEN_UINT) {
            uint32_t value;
            if (token_to_uint32(&curr_token, &value)) {
                argv[argc].type = ARG_UINT;
                argv[argc].val.num = value;
            } else {
                UART_write_string("Error parsing argument #");
                UART_write_int(argc);
                UART_write_line(" to int");
                msg_help();
                return;
            }
        }
    }

    for (uint32_t i = 0; i < CMD_COUNT; i++) {
        if (token_match(&cmd_token, cmds[i].name)) {
            switch (cmds[i].cmd_type) {
            case CMD_GEN_WAVE:
                if (argc == 2 && argv[0].type == ARG_UINT &&
                    argv[1].type == ARG_UINT) {
                    GENCTRL_function(cmds[i].wave_lut, argv[0].val.num,
                                     argv[1].val.num);
                } else {
                    UART_write_line("Error wrong type or number of arguments");
                    msg_help();
                    return;
                }
                break;
            case CMD_FUN:
                cmds[i].fun(argc, argv);
                break;
            }
        }
    }

    UART_write_line("Error unknown command");
}