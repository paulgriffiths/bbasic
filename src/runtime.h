/*  BBASIC, an interpreter for a subset of BBC BASIC II.
 *  Copyright (C) 2021 Paul Griffiths.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PG_BBASIC_INTERNAL_RUNTIME_H
#define PG_BBASIC_INTERNAL_RUNTIME_H

#include <stdbool.h>
#include <signal.h>
#include "value.h"

/* Opaque and incomplete struct definition */
struct statement;

/* Error codes */
enum basic_error {
    ERR_NO_ERROR = -1,
    ERR_ACCURACY_LOST = 23,
    ERR_ARGUMENTS = 31,
    ERR_ARRAY = 14,
    ERR_BAD_CALL = 30,
    ERR_BAD_DIM = 10,
    ERR_BAD_HEX = 28,
    ERR_BAD_KEY = 251,
    ERR_BAD_MODE = 25,
    ERR_BAD_PROGRAM = 0,
    ERR_BAD_STRING = 253,
    ERR_BLOCK = 218,
    ERR_BYTE = 2,
    ERR_CANT_MATCH_FOR = 33,
    ERR_CHANNEL = 222,
    ERR_DATA = 216,
    ERR_DIM_SPACE = 11,
    ERR_DIVIDE_ZERO = 18,
    ERR_STRING_RANGE = 8,
    ERR_EOF = 223,
    ERR_ESCAPE = 17,
    ERR_EXP_RANGE = 24,
    ERR_FILE = 219,
    ERR_FOR_VARIABLE = 34,
    ERR_HEADER = 217,
    ERR_INDEX = 3,
    ERR_KEY_IN_USE = 250,
    ERR_LOG_RANGE = 22,
    ERR_MISSING_COMMA = 5,
    ERR_MISSING_QUOTE = 9,
    ERR_MISSING_RPAREN = 27,
    ERR_MISSING_HASH = 45,
    ERR_MISTAKE = 4,
    ERR_NEGATIVE_ROOT = 21,
    ERR_NO_FN = 7,
    ERR_NO_FOR = 32,
    ERR_NO_GOSUB = 38,
    ERR_NO_PROC = 13,
    ERR_NO_REPEAT = 43,
    ERR_NO_ROOM = 0,
    ERR_NO_SUCH_FN_PROC = 29,
    ERR_NO_SUCH_LINE = 41,
    ERR_NO_SUCH_VARIABLE = 26,
    ERR_NO_TO = 36,
    ERR_NOT_LOCAL = 12,
    ERR_ON_RANGE = 40,
    ERR_ON_SYNTAX = 39,
    ERR_OUT_OF_DATA = 42,
    ERR_OUT_OF_RANGE = 1,
    ERR_SYNTAX = 220,
    ERR_STRING_TOO_LONG = 19,
    ERR_SUBSCRIPT = 15,
    ERR_SYNTAX_ERROR = 16,
    ERR_TOO_BIG = 20,
    ERR_TOO_MANY_FORS = 35,
    ERR_TOO_MANY_GOSUBS = 37,
    ERR_TOO_MANY_REPEATS = 44,
    ERR_TYPE_MISMATCH = 6,
};

/* Statment execution statuses - must be negative
 * because error codes are non-negative
 */
enum execution_status {
    STATUS_EXIT = -1,
    STATUS_OK = -2,
    STATUS_ERROR = -3
};

/* Branching statuses - must be negative
 * because line numbers are positive
 */
enum branch_status {
    START_OF_PROGRAM = -1,
    END_OF_PROGRAM = -2 
};

/* Runtime functions */
int line_add(const int number, struct statement * stmt);
int program_run(void);
int run_statements(struct statement * s);
void runtime_free(void);

/* Runtime stack functions */
struct value * runtime_stack_pop(void);
void runtime_stack_push(struct value * e);

/* Program counter functions */
struct statement * pc(void);
void set_pc(struct statement * stmt);

/* Open file functions */
void open_file_add(const int fd);
void open_files_close_all(void);
int open_file_get_ptr(const int fd);
void open_file_increment_ptr(const int fd);
void open_file_remove(const int fd);
int open_file_set_ptr(const int fd, const int ptr);

#ifdef ENABLE_ANSI_COLOURS
/* Colour function */
void set_colour_used(void);
#endif

/* Error functions */
void error_clear(void);
int error_code(void);
bool error_is_set(void);
int error_last_code(void);
int error_last_line(void);
int error_line(void);
void error_output(void);
const char * error_string(const enum basic_error code);
enum basic_error error_set(enum basic_error code);
void error_stmt_set(struct statement * s, struct statement * next);
void error_stmt_clear(void);

/* Interrupt flag */
extern volatile sig_atomic_t interrupt;

#endif  /* PG_BBASIC_INTERNAL_RUNTIME_H */
