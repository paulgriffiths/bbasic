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

#ifndef PG_BBASIC_INTERNAL_STATEMENTS_H
#define PG_BBASIC_INTERNAL_STATEMENTS_H

#include <stdbool.h>
#include "expr.h"
#include "addr_set.h"

/* Statement types */
enum statement_type {
    STATEMENT_ASSIGN = 1,
    STATEMENT_BPUT,
    STATEMENT_CLEAR,
    STATEMENT_CLOSE,
    STATEMENT_COLOUR,
    STATEMENT_DATA,
    STATEMENT_DEF_FN,
    STATEMENT_DEF_PROC,
    STATEMENT_DIM,
    STATEMENT_END,
    STATEMENT_ENDPROC,
    STATEMENT_FOR,
    STATEMENT_FN,
    STATEMENT_FN_RETURN,
    STATEMENT_GOSUB,
    STATEMENT_GOTO,
    STATEMENT_IF,
    STATEMENT_INPUT,
    STATEMENT_INPUTF,
    STATEMENT_LOCAL,
    STATEMENT_NEXT,
    STATEMENT_NULL,
    STATEMENT_ON_ERROR,
    STATEMENT_ON_GOTO,
    STATEMENT_ON_GOSUB,
    STATEMENT_PRINT,
    STATEMENT_PRINTF,
    STATEMENT_PROC,
    STATEMENT_READ,
    STATEMENT_REM,
    STATEMENT_REPEAT,
    STATEMENT_REPORT,
    STATEMENT_RESTORE,
    STATEMENT_RETURN,
    STATEMENT_STOP,
    STATEMENT_TRACE,
    STATEMENT_UNTIL
};

/* Print specifier types */
enum print_specifier {
    PRINT_APOSTROPHE,
    PRINT_SEMICOLON,
    PRINT_COMMA,
    PRINT_EXPR
};

#define STMT_NUM_EXPRS (2)
#define STMT_NUM_STMTS (2)

/* TRACE status constants */
enum trace_type {
    TRACE_ON,
    TRACE_OFF
};

/* Tagged union statement object */
struct statement {
    int line_number;
    enum statement_type type;

    struct value * v;
    struct expr * e[STMT_NUM_EXPRS];
    struct print_item * pl;
    struct statement * stmt[STMT_NUM_STMTS];
    struct statement * next;

    int (*exec)(struct statement *s);
};

/* An item in a print list */
struct print_item {
    enum print_specifier spec;
    struct expr * e;
    struct print_item * next;
};

/* General statement functions */
void push_function(void);
void reset_data_pointer(void);
void statements_cleanup(void);
int statement_execute(struct statement * stmt);
void statement_fixup(struct statement * stmt, struct statement * next);
void statement_free(struct statement * stmt);

/* Functions for working with lists of statements */
struct statement * statement_append(struct statement * head,
        struct statement * tail);

/* Individual statement constructors */
struct statement * statement_assign_new(struct expr * var, struct expr * e);
struct statement * statement_bput_new(struct expr * c, struct expr * e);
struct statement * statement_clear_new(void);
struct statement * statement_close_new(struct expr * c);
struct statement * statement_colour_new(struct expr * e);
struct statement * statement_data_new(struct value * v);
struct statement * statement_def_fn_new(char * id, struct expr * e);
struct statement * statement_def_proc_new(char * id, struct expr * e);
struct statement * statement_dim_new(struct expr * var);
struct statement * statement_endproc_new(void);
struct statement * statement_end_new(void);
struct statement * statement_fn_return_new(struct expr * e);
struct statement * statement_for_new(struct statement * asgn,
        struct expr * term, struct expr * step);
struct statement * statement_gosub_new(struct expr * e);
struct statement * statement_goto_new(struct expr * e);
struct statement * statement_if_new(struct expr * cond,
        struct statement * stmt_then, struct statement * stmt_else);
struct statement * statement_input_new(struct print_item * list, const bool line);
struct statement * statement_inputf_new(struct expr * c, struct expr * e);
struct statement * statement_local_new(struct expr * e);
struct statement * statement_next_new(struct expr * e);
struct statement * statement_null_new(void);
struct statement * statement_on_error_new(struct statement * s);
struct statement * statement_on_gosub_new(struct expr * e,
        struct expr * lines, struct statement * s);
struct statement * statement_on_goto_new(struct expr * e,
        struct expr * lines, struct statement * s);
struct statement * statement_print_new(struct print_item * list);
struct statement * statement_printf_new(struct expr * c, struct expr * e);
struct statement * statement_proc_new(char * id, struct expr * e);
struct statement * statement_read_new(struct expr * vars);
struct statement * statement_rem_new(char * s);
struct statement * statement_repeat_new(struct statement * s);
struct statement * statement_report_new(void);
struct statement * statement_restore_new(struct expr * e);
struct statement * statement_return_new(void);
struct statement * statement_stop_new(void);
struct statement * statement_trace_new(enum trace_type type, struct expr * e);
struct statement * statement_until_new(struct expr * e);

/* Print list functions */
struct print_item * print_list_expr_append(struct print_item * head,
        struct expr * e);
struct print_item * print_list_item_append(struct print_item * head,
        struct print_item * tail);
struct print_item * print_list_specifier_append(struct print_item * head,
        const enum print_specifier spec);
void print_list_free(struct print_item * list);

/* Print count function */
int32_t print_count(void);

#endif  /* PG_BBASIC_INTERNAL_STATEMENTS_H */
