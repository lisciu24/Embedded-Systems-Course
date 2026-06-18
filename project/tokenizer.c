#include "tokenizer.h"
#ifndef _TOK_TEST_
#include "LPC17xx.h"
#include "generator_ctrl.h"
#include "uart.h"
#else
#include "test/generator_ctrl_mock.h"
#include "test/uart_mock.h"
#endif

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

typedef enum {
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_END,
    TOKEN_EMPTY,
    TOKEN_UNKNOWN,
    TOKEN_CMD
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

static const char *const PARSE_ERR_MSG[PARSE_ERROR + 1] = {
    [PARSE_OK] = "Parsed OK",
    [PARSE_STRING_INVALID_CHAR] = "Invalid character in string",
    [PARSE_NUM_INVALID_BASE] = "Invalid numeric base",
    [PARSE_NUM_INVALID_CHAR] = "Invalid character in number",
    [PARSE_NUM_INVALID_TOKEN_TYPE] = "Invalid token type for number",
    [PARSE_NUM_OVERFLOW] = "Numeric overflow",
    [PARSE_ERROR] = "Invalid character"};

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

typedef enum { CMD_GEN_WAVE, CMD_FUN, CMD_FUN_SEQ } cmd_type_t;

typedef void (*cmd_fun_t)(uint32_t, arg_t[]);
typedef uint32_t (*cmd_fun_seq_t)(uint32_t, arg_t[]);

typedef struct {
    const char *name;
    cmd_type_t cmd_type;

    union {
        const uint16_t *wave_lut;
        cmd_fun_t fun;
        cmd_fun_seq_t fun_seq;
    };
} cmd_t;

static uint32_t token_match(const token_t *token, const char *str) {
    if (token->type != TOKEN_STRING || token->length != strlen(str))
        return 0;
    return strncmp(token->start, str, token->length) == 0;
}

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

static uint32_t bmp_row_idx = 0;
static uint32_t cmd_bitmap(uint32_t argc, arg_t argv[]) {
    uint32_t err = 0;
    for (uint32_t i = 0; i < argc; i++) {
        switch (argv[i].type) {
        case ARG_NUMBER:
            if (argv[i].val.num < 1 << BITMAP_SIZE) {
                GENCTRL_load_bitmap_row(argv[i].val.num, bmp_row_idx);
                if (++bmp_row_idx == BITMAP_SIZE) {
                    bmp_row_idx = 0;
                    GENCTRL_bitmap();
                    return 1;
                }
            } else {
                err = 1;
            }
            break;
        case ARG_STRING:
            if (token_match(&argv[i].val.str, "Q")) {
                bmp_row_idx = 0;
                UART_write_line("Quiting BMP mode");
                return 1;
            } else {
                err = 1;
            }
            break;
        default:
            err = 1;
            break;
        }
        if (err)
            break;
    }

    if (err) {
        UART_write_string("Expected integer value in range [0, ");
        UART_write_int(2 << BITMAP_SIZE);
        UART_write_line("]");
        UART_write_line("Send 'Q' to quit BMP mode");
    }

    UART_write_int(bmp_row_idx + 1);
    UART_write_string("/");
    UART_write_int(BITMAP_SIZE);
    UART_write_string(" > ");

    return 0;
}

static const cmd_t cmds[] = {
    {"SIN", CMD_GEN_WAVE, .wave_lut = sin_lut},
    {"SQUARE", CMD_GEN_WAVE, .wave_lut = square_lut},
    {"TRIANGLE", CMD_GEN_WAVE, .wave_lut = triangle_lut},
    {"HELP", CMD_FUN, .fun = cmd_help},
    {"START", CMD_FUN, .fun = cmd_start},
    {"STOP", CMD_FUN, .fun = cmd_stop},
    {"BITMAP", CMD_FUN_SEQ, .fun_seq = cmd_bitmap}};

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

        cursor++;
    }

    return value;
}

#define CMD_COUNT (sizeof(cmds) / sizeof(cmd_t))

typedef enum lexer_state_e {
    LEX_STATE_BEGIN,
    LEX_STATE_READ_STRING,
    LEX_STATE_READ_NUMBER,
    LEX_STATE_READ_ERROR
} lexer_state_t;

static const char *get_next_token(const char *cursor, token_t *token) {
    lexer_state_t state = LEX_STATE_BEGIN;
    token->start = cursor;
    token->length = 0;
    token->err = PARSE_OK;

    while (*cursor != '\0' && *cursor != ':' && *cursor != ';') {
        const char ch = *cursor;

        switch (state) {
        case LEX_STATE_BEGIN:
            if (isdigit(ch)) {
                state = LEX_STATE_READ_NUMBER;
                token->type = TOKEN_NUMBER;
            } else if (isalpha(ch)) {
                state = LEX_STATE_READ_STRING;
                token->type = TOKEN_STRING;
            } else {
                state = LEX_STATE_READ_ERROR;
                token->type = TOKEN_UNKNOWN;
                token->err = PARSE_ERROR;
                token->length = cursor - token->start;
            }
            break;

        case LEX_STATE_READ_STRING:
            if (!isalnum(ch)) {
                state = LEX_STATE_READ_ERROR;
                token->err = PARSE_STRING_INVALID_CHAR;
                token->length = cursor - token->start;
            }
            break;

        case LEX_STATE_READ_NUMBER:
            if (!isxdigit(ch) && cursor - token->start != 1) {
                state = LEX_STATE_READ_ERROR;
                token->err = PARSE_NUM_INVALID_CHAR;
                token->length = cursor - token->start;
            }
            break;
        case LEX_STATE_READ_ERROR:
            break;
        }

        cursor++;
    }

    if (state == LEX_STATE_BEGIN) {
        switch (*cursor) {
        case ':':
            token->type = TOKEN_EMPTY;
            break;
        case '\0':
            token->type = TOKEN_END;
            break;
        case ';':
            token->type = TOKEN_CMD;
        }
    } else if (state != LEX_STATE_READ_ERROR) {
        token->length = cursor - token->start;
    }

    return *cursor ? cursor + 1 : cursor;
}

static void token_parse_err_msg(const char *cmd, uint32_t token_num,
                                const token_t *token) {
    if (token->err == PARSE_OK) {
        UART_debug("token_parse_err_msg called when token OK!");
        return;
    }
    UART_write_string("Error parsing token #");
    UART_write_int(token_num);
    UART_write_string(" ");
    UART_write_line(PARSE_ERR_MSG[token->err]);
    UART_write_line(cmd);
    const char *tmp = cmd;
    while (tmp != token->start + token->length) {
        UART_write_string("-");
        tmp++;
    }
    UART_write_line("^");
}

void CMD_parse_bmp_row(const char *cmd) {}

static token_t cmd_token = {TOKEN_EMPTY};
static char cmd_buff[16];

void CMD_parse(const char *cmd) {
    const char *cursor = cmd;
    token_t token;
    uint32_t parse_arg_error = 0;
    uint32_t parse_cmd_error = 0;

    if (cmd_token.type == TOKEN_EMPTY) {
        cursor = get_next_token(cursor, &token);
        if (token.err != PARSE_OK || token.type != TOKEN_STRING ||
            token.length > 15) {
            UART_write_line("Error parsing command!");
            parse_cmd_error = 1;
        } else {
            strncpy(cmd_buff, token.start, token.length);
            cmd_buff[token.length] = '\0';
            cmd_token = token;
            cmd_token.start = cmd_buff;
        }
    }

    uint32_t argc = 0;
    arg_t argv[MAX_ARG_COUNT];

    for (; argc < MAX_ARG_COUNT; argc++) {
        cursor = get_next_token(cursor, &token);
        if (token.type == TOKEN_END)
            break;

        if (token.err != PARSE_OK) {
            token_parse_err_msg(cmd, argc, &token);
            parse_arg_error = 1;
            break;
        }

        if (token.type == TOKEN_STRING) {
            argv[argc].type = ARG_STRING;
            argv[argc].val.str = token;
        } else if (token.type == TOKEN_NUMBER) {
            uint32_t value = token_to_uint32(&token);
            if (token.err == PARSE_OK) {
                argv[argc].type = ARG_NUMBER;
                argv[argc].val.num = value;
            } else {
                token_parse_err_msg(cmd, argc, &token);
                parse_arg_error = 1;
                break;
            }
        } else if (token.type == TOKEN_EMPTY) {
            argv[argc].type = ARG_NONE;
        }
    }

    if (parse_cmd_error)
        return;

    for (uint32_t i = 0; i < CMD_COUNT; i++) {
        if (token_match(&cmd_token, cmds[i].name)) {
            if (parse_arg_error && cmds[i].cmd_type != CMD_FUN_SEQ) {
                cmd_token.type = TOKEN_EMPTY;
                return;
            }

            switch (cmds[i].cmd_type) {
            case CMD_GEN_WAVE: {
                uint32_t args_ok = 1;
                if (argv[0].type != ARG_NUMBER) {
                    args_ok = 0;
                    UART_write_line("Error: invalid frequency (first arg), "
                                    "expected number!");
                }
                if (argv[1].type != ARG_NUMBER) {
                    args_ok = 0;
                    UART_write_line("Error: invalid amplitude (second arg), "
                                    "expected number!");
                }

                if (args_ok)
                    GENCTRL_function(cmds[i].wave_lut, argv[1].val.num,
                                     argv[0].val.num);
                cmd_token.type = TOKEN_EMPTY;
            } break;
            case CMD_FUN:
                cmds[i].fun(argc, argv);
                cmd_token.type = TOKEN_EMPTY;
                break;
            case CMD_FUN_SEQ:
                if (cmds[i].fun_seq(argc, argv))
                    cmd_token.type = TOKEN_EMPTY;
                break;
            }

            return;
        }
    }
    cmd_token.type = TOKEN_EMPTY;
    UART_write_line("Error: unknown command");
}
