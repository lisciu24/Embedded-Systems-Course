#include "tokenizer.h"
#include "LPC17xx.h"
#include "generator_ctrl.h"
#include "uart.h"
#include "waves_lut.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
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

typedef enum {
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_END,
    TOKEN_EMPTY,
    TOKEN_UNKNOWN
} token_type_t;

typedef enum {
    PARSE_OK,

    PARSE_STRING_INVALID_CHAR,

    PARSE_NUM_INVALID_BASE,
    PARSE_NUM_INVALID_CHAR,
    PARSE_NUM_INVALID_TOKEN_TYPE,
    PARSE_NUM_OVERFLOW,

    PARSE_ERROR // MUST BE LAST ONE
} token_parse_err_t;

static const char *const PARSE_ERR_MSG[PARSE_ERROR] = {
    [PARSE_OK] = "Parsed OK",
    [PARSE_STRING_INVALID_CHAR] = "Invalid character in string",
    [PARSE_NUM_INVALID_BASE] = "Invalid numeric base",
    [PARSE_NUM_INVALID_CHAR] = "Invalid character in number",
    [PARSE_NUM_INVALID_TOKEN_TYPE] = "Invalid token type for number",
    [PARSE_NUM_OVERFLOW] = "Numeric overflow"};

typedef struct {
    token_type_t type;
    const char *start; // Pointer to first character
    uint32_t length;   // length of token
    token_parse_err_t err;
} token_t;

typedef enum { ARG_NONE, ARG_STRING, ARG_NUMBER } arg_type_t;

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
static void cmd_bitmap(uint32_t argc, arg_t argv[]) { GENCTRL_bitmap(); }

static const cmd_t cmds[] = {
    {"SIN", CMD_GEN_WAVE, .wave_lut = sin_lut},
    {"SQUARE", CMD_GEN_WAVE, .wave_lut = square_lut},
    {"TRIANGLE", CMD_GEN_WAVE, .wave_lut = triangle_lut},
    {"HELP", CMD_FUN, .fun = cmd_help},
    {"START", CMD_FUN, .fun = cmd_start},
    {"STOP", CMD_FUN, .fun = cmd_stop},
    {"BITMAP", CMD_FUN, .fun = cmd_bitmap}};

#define CMD_COUNT (sizeof(cmds) / sizeof(cmd_t))
#define MAX_ARG_COUNT 2

typedef enum {
    STATE_BEGIN,
    STATE_READ_STRING,
    STATE_READ_NUMBER,
    STATE_READ_ERROR
} lexer_state_t;

static uint32_t token_match(const token_t *token, const char *str) {
    if (token->type != TOKEN_STRING || token->length != strlen(str))
        return 0;
    return strncmp(token->start, str, token->length) == 0;
}

static uint32_t token_to_uint32(token_t *token) {
    if (token->type != TOKEN_NUMBER) {
        token->err = PARSE_NUM_INVALID_TOKEN_TYPE;
        return 0;
    }
    uint32_t base = 10;
    uint32_t value = 0;

    const char *cursor = token->start;
    if (token->length > 2 && *cursor == '0') {
        base = 8;
        cursor++;
        if (*cursor == 'b' || *cursor == 'x') {
            base = *cursor++ == 'b' ? 2 : 16;
        } else if (!(*cursor >= '0' && *cursor <= '7')) {
            token->length = cursor - token->start;
            token->err = PARSE_NUM_INVALID_BASE;
            return 0;
        }
    }

    while (cursor != token->start + token->length && token->err == PARSE_OK) {
        uint32_t p_value = value;
        switch (base) {
        case 2:
            if (*cursor == '0' || *cursor == '1') {
                value = value * 2 + (*cursor - '0');
            } else {
                token->err = PARSE_NUM_INVALID_CHAR;
                token->length = cursor - token->start;
            }
            break;

        case 8:
            if (*cursor >= '0' && *cursor <= '7') {
                value = value * 8 + (*cursor - '0');
            } else {
                token->err = PARSE_NUM_INVALID_CHAR;
                token->length = cursor - token->start;
            }
            break;

        case 10:
            if (isdigit(*cursor)) {
                value = value * 10 + (*cursor - '0');
            } else {
                token->err = PARSE_NUM_INVALID_CHAR;
                token->length = cursor - token->start;
            }
            break;

        case 16:
            if (isdigit(*cursor)) {
                value = value * 16 + (*cursor - '0');
            } else if (isxdigit(*cursor)) {
                value = value * 16 + (toupper(*cursor) - 'A') + 10;
            } else {
                token->err = PARSE_NUM_INVALID_CHAR;
                token->length = cursor - token->start;
            }
            break;
        }
        if (value < p_value) {
            token->err = PARSE_NUM_OVERFLOW;
            token->length = cursor - token->start;
        }
    }

    return value;
}

static const char *get_next_token(const char *cursor, token_t *token) {
    lexer_state_t state = STATE_BEGIN;
    token->start = cursor;
    token->length = 0;
    token->err = PARSE_OK;

    while (*cursor != '\0' && *cursor != ':') {
        const char ch = *cursor;

        switch (state) {
        case STATE_BEGIN:
            if (isdigit(ch)) {
                state = STATE_READ_NUMBER;
                token->type = TOKEN_NUMBER;
            } else if (isalpha(ch)) {
                state = STATE_READ_STRING;
                token->type = TOKEN_STRING;
            } else {
                state = STATE_READ_ERROR;
                token->type = TOKEN_UNKNOWN;
                token->err = PARSE_ERROR;
            }
            break;

        case STATE_READ_STRING:
            if (!isalnum(ch)) {
                state = STATE_READ_ERROR;
                token->err = PARSE_STRING_INVALID_CHAR;
                token->length = cursor - token->start;
            }
            break;

        case STATE_READ_NUMBER:
            if (!isxdigit(ch) && cursor - token->start != 1) {
                state = STATE_READ_ERROR;
                token->err = PARSE_NUM_INVALID_CHAR;
                token->length = cursor - token->start;
            }
            break;
        case STATE_READ_ERROR:
            break;
        }

        cursor++;
    }

    if (state == STATE_BEGIN) {
        token->type = *cursor ? TOKEN_EMPTY : TOKEN_END;
    } else if (state != STATE_READ_ERROR) {
        token->length = cursor - token->start;
    }

    return cursor + 1;
}

static void token_parse_err_msg(const char *cmd, uint32_t token_num,
                                const token_t *token) {
    if (token->err == PARSE_OK) {
        UART_debug("token_parse_err_msg called when token OK!");
        return;
    }
    UART_write_string("Error parsing token #");
    UART_write_int(token_num);
    UART_write_line(PARSE_ERR_MSG[token->err]);
    UART_write_line(cmd);
    const char *tmp = cmd;
    while (tmp != token->start + token->length) {
        UART_write_string("-");
        tmp++;
    }
    UART_write_line("^");
}

void CMD_parse(const char *cmd) {
    const char *cursor = cmd;
    token_t token;
    uint32_t parse_error = 0;

    cursor = get_next_token(cursor, &token);
    if (token.err != PARSE_OK || token.type != TOKEN_STRING) {
        UART_write_line("Error parsing command!");
        parse_error = 1;
    }

    token_t cmd_token = token;

    uint32_t argc = 0;
    arg_t argv[MAX_ARG_COUNT] = {};

    for (argc = 0; argc < MAX_ARG_COUNT; argc++) {
        cursor = get_next_token(cursor, &token);
        if (token.type == TOKEN_END)
            break;

        if (token.err != PARSE_OK) {
            token_parse_err_msg(cmd, argc, &token);
            parse_error = 1;
            break;
        }

        if (token.type == TOKEN_STRING) {
            argv[argc].type = ARG_STRING;
            argv[argc].val.str = token;
        } else if (token.type == TOKEN_NUMBER) {
            uint32_t value = token_to_uint32(&token);
            if (token.err != PARSE_OK) {
                argv[argc].type = ARG_NUMBER;
                argv[argc].val.num = value;
            } else {
                token_parse_err_msg(cmd, argc, &token);
                parse_error = 1;
                break;
            }
        } else if (token.type == TOKEN_EMPTY) {
            argv[argc].type = ARG_NONE;
        }
    }

    if (parse_error)
        return;

    for (uint32_t i = 0; i < CMD_COUNT; i++) {
        if (token_match(&cmd_token, cmds[i].name)) {
            switch (cmds[i].cmd_type) {
            case CMD_GEN_WAVE: {
                uint32_t args_ok = 1;
                if (argc != 2) {
                    args_ok = 0;
                    UART_write_line(
                        "Error: invalid number of arguments, expected 2!");
                }
                if (argv[0].type != ARG_NUMBER) {
                    args_ok = 0;
                    UART_write_line(
                        "Error: invalid frequency, expected number!");
                }
                if (argv[1].type != ARG_NUMBER) {
                    args_ok = 0;
                    UART_write_line(
                        "Error: invalid amplitude, expected number!");
                }

                if (args_ok)
                    GENCTRL_function(cmds[i].wave_lut, argv[1].val.num,
                                     argv[0].val.num);
            } break;
            case CMD_FUN:
                cmds[i].fun(argc, argv);
                break;
            }

            return;
        }
    }
    UART_write_line("Error: unknown command");
}