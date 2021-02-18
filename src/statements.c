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

/* Functions for creating and executing program statements */

#include "internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "statements.h"
#include "expr.h"
#include "value.h"
#include "util.h"
#include "runtime.h"
#include "symbols.h"
#include "line_map.h"
#include "data_map.h"
#include "addr_set.h"
#include "stack_addr.h"
#include "options.h"
#include "colours.h"

#define BUFFER_SIZE (257)
#define MAX_LINE_LEN (256)

/* Return address stacks */
static struct stack_addr fn_stack;
static struct stack_addr for_stack;
static struct stack_addr gosub_stack;
static struct stack_addr proc_stack;
static struct stack_addr repeat_stack;
static struct stack_addr return_stack;

/* TRACE statement state */
static bool trace_on;       /* TRACE on/off */
static int32_t trace_last_line; /* Last line we traced, to avoid repeition */
static int32_t trace_threshold; /* Line threshold above which we won't trace */

/* Value of COUNT pseudo-variable */
static int32_t pcount;

/* List of DATA items and current pointer */
static struct value * data_items;
static struct value * data_ptr;

/* Static function declarations */
static int stmt_exec_assign(struct statement * s);
static int stmt_exec_bput(struct statement * s);
static int stmt_exec_clear(struct statement * s);
static int stmt_exec_close(struct statement * s);
static int stmt_exec_colour(struct statement * s);
static int stmt_exec_dim(struct statement * s);
static int stmt_exec_end(struct statement * s);
static int stmt_exec_endproc(struct statement * s);
static int stmt_exec_fn_return(struct statement * s);
static int stmt_exec_for(struct statement * s);
static int stmt_exec_gosub(struct statement * s);
static int stmt_exec_goto(struct statement * s);
static int stmt_exec_if(struct statement * s);
static int stmt_exec_input(struct statement * s);
static int stmt_exec_inputf(struct statement * s);
static int stmt_exec_local(struct statement * s);
static int stmt_exec_next(struct statement * s);
static int stmt_exec_null(struct statement * s);
static int stmt_exec_on(struct statement * s, bool gosub);
static int stmt_exec_on_error(struct statement * s);
static int stmt_exec_on_gosub(struct statement * s);
static int stmt_exec_on_goto(struct statement * s);
static int stmt_exec_print(struct statement * s);
static int stmt_exec_printf(struct statement * s);
static int stmt_exec_proc(struct statement * s);
static int stmt_exec_read(struct statement * s);
static int stmt_exec_repeat(struct statement * s);
static int stmt_exec_report(struct statement * s);
static int stmt_exec_restore(struct statement * s);
static int stmt_exec_return(struct statement * s);
static int stmt_exec_stop(struct statement * s);
static int stmt_exec_trace(struct statement * s);
static int stmt_exec_until(struct statement * s);

static int advance_file_ptr(struct expr * c, struct expr * e);
static int assign_expr(struct expr * var, struct expr * e);
static int assign_value(struct expr * var, struct value * v);
static struct value * eval_array_indices(struct expr * var);
static struct print_item * print_item_new(enum print_specifier spec,
        struct expr * e);
static struct statement * create(enum statement_type type);
static struct statement * for_stack_peek(struct expr * e);
static void free_internal(struct statement * stmt,
        struct addr_set * set);
static int increment_var(struct statement * parent,
        struct expr * var, struct value * inc);
static void update_line_numbers(struct statement *s, const int line_number);


/*********************************************************************
 *                                                                   *
 * Public general functions                                          *
 *                                                                   *
 *********************************************************************/

/* Pushes the current stack pointer onto the function stack */
void
push_function(void) {
    stack_addr_push(&fn_stack, pc());
}

/* Resets the data pointer to the head of the data items list */
void
reset_data_pointer(void) {
    data_ptr = data_items;
}

/* Cleans up any resources associated with the statements
 * runtime environment, but not attributable to any specific
 * statement.
 */
void
statements_cleanup(void) {
    stack_addr_free(&fn_stack);
    stack_addr_free(&for_stack);
    stack_addr_free(&gosub_stack);
    stack_addr_free(&proc_stack);
    stack_addr_free(&repeat_stack);
    stack_addr_free(&return_stack);
    value_free(data_items);
    data_map_free();
}

/* Appends tail to head, and returns head. If head is NULL,
 * tail is returned.
 */
struct statement *
statement_append(struct statement * first, struct statement * second) {
    if ( first == NULL ) {
        /* Enable calling on NULL list */
        return second;
    }

    struct statement * current = first;
    while ( current->next ) {
        current = current->next;
    }
    current->next = second;

    return first;
}

/* Executes a single statement */
int
statement_execute(struct statement * s) {
    /* Output TRACE if TRACEing is active */
    if ( trace_on && s->line_number <= trace_threshold &&
            s->line_number != trace_last_line ) {
        /* Update last traced line, so that if a line contains more than
         * one statement, we only TRACE it once per line execution.
         */
        trace_last_line = s->line_number;
        fprintf(stderr, "[%d] ", s->line_number);
    }

    /* Execute statement */
    return s->exec ? s->exec(s) : STATUS_OK;
}

/* Fixes up sub-statements and sub-statement branches with the
 * correct line numbers and next statement pointers.
 */
void
statement_fixup(struct statement * s, struct statement * next) {
    for ( size_t i = 0; i < STMT_NUM_STMTS; i++ ) {
        if ( !s->stmt[i] ) {
            break;
        }

        update_line_numbers(s->stmt[i], s->line_number);
        s->stmt[i] = statement_append(s->stmt[i], next);
    }

    /* Add any DATA values to the data_items list, and remove
     * them from the statement itself, to avoid double freeing.
     * Note we can't do this from the statement constructor,
     * since the line numbers aren't available there.
     */
    if ( s->type == STATEMENT_DATA ) {
        data_items = value_append(data_items, s->v);
        data_map_add(s->line_number, s->v);
        s->v = NULL;
    }
}

/* Recursively frees the resources associated with a statement
 * and any statements it links to.
 */
void
statement_free(struct statement * s) {
    struct addr_set * set = addr_set_new();
    free_internal(s, set);
    addr_set_free(set);
}


/*********************************************************************
 *                                                                   *
 * Constructor functions                                             *
 *                                                                   *
 *********************************************************************/

/* Constructs a new assignment statement */
struct statement *
statement_assign_new(struct expr * var, struct expr * e) {
    struct statement * stmt = create(STATEMENT_ASSIGN);
    stmt->e[0] = var;
    stmt->e[1] = e;
    stmt->exec = stmt_exec_assign;
    return stmt;
}

/* Constructs a new BPUT# statement */
struct statement *
statement_bput_new(struct expr * c, struct expr * b) {
    struct statement * stmt = create(STATEMENT_BPUT);
    stmt->e[0] = c;
    stmt->e[1] = b;
    stmt->exec = stmt_exec_bput;
    return stmt;
}

/* Constructs a new CLEAR statement */
struct statement *
statement_clear_new(void) {
    struct statement * stmt = create(STATEMENT_CLEAR);
    stmt->exec = stmt_exec_clear;
    return stmt;
}

/* Constructs a new CLOSE# statement */
struct statement *
statement_close_new(struct expr * c) {
    struct statement * stmt = create(STATEMENT_CLOSE);
    stmt->e[0] = c;
    stmt->exec = stmt_exec_close;
    return stmt;
}

/* Constructs a new COLOUR statement */
struct statement *
statement_colour_new(struct expr * e) {
    struct statement * stmt = create(STATEMENT_COLOUR);
    stmt->e[0] = e;
    stmt->exec = stmt_exec_colour;
    return stmt;
}

/* Constructs a new DATA statement */
struct statement *
statement_data_new(struct value * v) {
    struct statement * stmt = create(STATEMENT_DATA);
    stmt->v = v;
    return stmt;
}

/* Constructs a new DEF PROC statement */
struct statement *
statement_def_proc_new(char * id, struct expr * vars) {
    struct statement * stmt = create(STATEMENT_DEF_PROC);
    stmt->v = value_string_new(id);
    stmt->e[0] = vars;
    return stmt;
}

/* Constructs a new DEF FN statement */
struct statement *
statement_def_fn_new(char * id, struct expr * vars) {
    struct statement * stmt = create(STATEMENT_DEF_FN);
    stmt->v = value_string_new(id);
    stmt->e[0] = vars;
    return stmt;
}

/* Constructs a new DIM statement */
struct statement *
statement_dim_new(struct expr * var) {
    struct statement * stmt = create(STATEMENT_DIM);
    stmt->e[0] = var;
    stmt->exec = stmt_exec_dim;
    return stmt;
}

/* Constructs a new END statement */
struct statement *
statement_end_new(void) {
    struct statement * stmt = create(STATEMENT_END);
    stmt->exec = stmt_exec_end;
    return stmt;
}

/* Constructs a new ENDPROC statement */
struct statement *
statement_endproc_new(void) {
    struct statement * stmt = create(STATEMENT_ENDPROC);
    stmt->exec = stmt_exec_endproc;
    return stmt;
}

/* Constructs a new FN return statement */
struct statement *
statement_fn_return_new(struct expr * e) {
    struct statement * stmt = create(STATEMENT_FN_RETURN);
    stmt->e[0] = e;
    stmt->exec = stmt_exec_fn_return;
    return stmt;
}

/* Constructs a new FOR statement */
struct statement *
statement_for_new(struct statement * asgn,
        struct expr * term, struct expr * step) {
    struct statement * stmt = create(STATEMENT_FOR);
    stmt->e[0] = term;
    stmt->e[1] = step;
    stmt->stmt[0] = asgn;
    stmt->exec = stmt_exec_for;
    return stmt;
}

/* Constructs a new GOSUB statement */
struct statement *
statement_gosub_new(struct expr * e) {
    struct statement * stmt = create(STATEMENT_GOSUB);
    stmt->e[0] = e;
    stmt->exec = stmt_exec_gosub;
    return stmt;
}

/* Constructs a new GOTO statement */
struct statement *
statement_goto_new(struct expr * e) {
    struct statement * stmt = create(STATEMENT_GOTO);
    stmt->e[0] = e;
    stmt->exec = stmt_exec_goto;
    return stmt;
}

/* Constructs a new IF statement */
struct statement *
statement_if_new(struct expr * cond,
        struct statement * stmt_then, struct statement * stmt_else) {
    struct statement * stmt = create(STATEMENT_IF);
    stmt->e[0] = cond;
    stmt->stmt[0] = stmt_then;
    stmt->stmt[1] = stmt_else;
    stmt->exec = stmt_exec_if;
    return stmt;
}

/* Constructs a new INPUT statement */
struct statement *
statement_input_new(struct print_item * list, const bool line) {
    struct statement * stmt = create(STATEMENT_INPUT);
    if ( line ) {
        stmt->v = value_int_new(1);
    }
    stmt->pl = list;
    stmt->exec = stmt_exec_input;
    return stmt;
}

/* Constructs a new INPUT# statement */
struct statement *
statement_inputf_new(struct expr * c, struct expr * e) {
    struct statement * stmt = create(STATEMENT_INPUTF);
    stmt->e[0] = c;
    stmt->e[1] = e;
    stmt->exec = stmt_exec_inputf;
    return stmt;
}

/* Constructs a new LOCAL statement */
struct statement *
statement_local_new(struct expr * e) {
    struct statement * stmt = create(STATEMENT_LOCAL);
    stmt->e[0] = e;
    stmt->exec = stmt_exec_local;
    return stmt;
}

/* Constructs a new NEXT statement */
struct statement *
statement_next_new(struct expr * e) {
    struct statement * stmt = create(STATEMENT_NEXT);
    stmt->e[0] = e;
    stmt->exec = stmt_exec_next;
    return stmt;
}

/* Constructs a new null statement */
struct statement *
statement_null_new(void) {
    struct statement * stmt = create(STATEMENT_NULL);
    stmt->exec = stmt_exec_null;
    return stmt;
}

/* Constructs a new ON ERROR statement */
struct statement *
statement_on_error_new(struct statement * s) {
    struct statement * stmt = create(STATEMENT_ON_ERROR);
    stmt->stmt[0] = s;
    stmt->exec = stmt_exec_on_error;
    return stmt;
}

/* Constructs a new ON GOSUB statement */
struct statement *
statement_on_gosub_new(struct expr * e, struct expr * lines, struct statement * s) {
    struct statement * stmt = create(STATEMENT_ON_GOSUB);
    stmt->e[0] = e;
    stmt->e[1] = lines;
    stmt->stmt[0] = s;
    stmt->exec = stmt_exec_on_gosub;
    return stmt;
}

/* Constructs a new ON GOTO statement */
struct statement *
statement_on_goto_new(struct expr * e, struct expr * lines, struct statement * s) {
    struct statement * stmt = create(STATEMENT_ON_GOTO);
    stmt->e[0] = e;
    stmt->e[1] = lines;
    stmt->stmt[0] = s;
    stmt->exec = stmt_exec_on_goto;
    return stmt;
}

/* Constructs a new PRINT statement */
struct statement *
statement_print_new(struct print_item * list) {
    struct statement * stmt = create(STATEMENT_PRINT);
    stmt->pl = list;
    stmt->exec = stmt_exec_print;
    return stmt;
}

/* Constructs a new PRINT# statement */
struct statement *
statement_printf_new(struct expr * c, struct expr * e) {
    struct statement * stmt = create(STATEMENT_PRINTF);
    stmt->e[0] = c;
    stmt->e[1] = e;
    stmt->exec = stmt_exec_printf;
    return stmt;
}

/* Constructs a new PROC statement */
struct statement *
statement_proc_new(char * id, struct expr * e) {
    struct statement * stmt = create(STATEMENT_PROC);
    stmt->v = value_string_new(id);
    stmt->e[0] = e;
    stmt->exec = stmt_exec_proc;
    return stmt;
}

/* Constructs a new READ statement */
struct statement *
statement_read_new(struct expr * vars) {
    struct statement * stmt = create(STATEMENT_REM);
    stmt->e[0] = vars;
    stmt->exec = stmt_exec_read;
    return stmt;
}

/* Constructs a new REM statement */
struct statement *
statement_rem_new(char * s) {
    struct statement * stmt = create(STATEMENT_REM);
    stmt->v = value_string_new(s);
    return stmt;
}

/* Constructs a new REPEAT statement */
struct statement *
statement_repeat_new(struct statement * s) {
    struct statement * stmt = create(STATEMENT_REPEAT);
    stmt->stmt[0] = s;
    stmt->exec = stmt_exec_repeat;
    return stmt;
}

/* Constructs a new RESTORE statement */
struct statement *
statement_restore_new(struct expr * e) {
    struct statement * stmt = create(STATEMENT_RESTORE);
    stmt->e[0] = e;
    stmt->exec = stmt_exec_restore;
    return stmt;
}

/* Constructs a new REPORT statement */
struct statement *
statement_report_new(void) {
    struct statement * stmt = create(STATEMENT_REPORT);
    stmt->exec = stmt_exec_report;
    return stmt;
}

/* Constructs a new RETURN statement */
struct statement *
statement_return_new(void) {
    struct statement * stmt = create(STATEMENT_RETURN);
    stmt->exec = stmt_exec_return;
    return stmt;
}

/* Constructs a new STOP statement */
struct statement *
statement_stop_new(void) {
    struct statement * stmt = create(STATEMENT_STOP);
    stmt->exec = stmt_exec_stop;
    return stmt;
}

/* Constructs a new TRACE statement */
struct statement *
statement_trace_new(enum trace_type type, struct expr * e) {
    struct statement * stmt = create(STATEMENT_TRACE);
    stmt->v = value_int_new(type);
    stmt->exec = stmt_exec_trace;

    /* The optional expression indicates the highest line number to
     * trace. If it is missing, set it to INT_MAX, so that we trace
     * all lines.
     */
    stmt->e[0] = e ? e : expr_int_new(INT_MAX);

    return stmt;
}

/* Constructs a new UNTIL statement */
struct statement *
statement_until_new(struct expr * e) {
    struct statement * stmt = create(STATEMENT_UNTIL);
    stmt->e[0] = e;
    stmt->exec = stmt_exec_until;
    return stmt;
}


/*********************************************************************
 *                                                                   *
 * Print list functions                                              *
 *                                                                   *
 *********************************************************************/

/* Allocates a new print list specifier and appends it to head. If
 * head is null, the new specifier is returned by itself.
 */
struct print_item *
print_list_specifier_append(struct print_item * head, const enum print_specifier spec) {
    struct print_item * item = print_item_new(spec, NULL);

    if ( !head ) {
        return item;
    }

    struct print_item * current = head;
    while ( current->next ) {
        current = current->next;
    }
    current->next = item;

    return head;
}

/* Appends tail to head, and returns head. If head is NULL,
 * tail is returned.
 */
struct print_item *
print_list_item_append(struct print_item * head, struct print_item * tail) {
    if ( !head ) {
        /* Enable calling on NULL list */
        return tail;
    }

    struct print_item * current = head;
    while ( current->next ) {
        current = current->next;
    }
    current->next = tail;

    return head;
}

/* Allocates a new print list expression and appends it to head. If
 * head is null, the new expression is returned by itself.
 */
struct print_item *
print_list_expr_append(struct print_item * head, struct expr * e) {
    struct print_item * item = print_item_new(PRINT_EXPR, e);

    if ( !head ) {
        return item;
    }

    struct print_item * current = head;
    while ( current->next ) {
        current = current->next;
    }
    current->next = item;

    return head;
}

/* Frees a print list */
void
print_list_free(struct print_item * list) {
    struct print_item * current = list;
    while ( list ) {
        struct print_item * tmp = list->next;
        expr_free(list->e);
        free(list);
        list = tmp;
    }
}


/*********************************************************************
 *                                                                   *
 * PRINT count function                                              *
 *                                                                   *
 *********************************************************************/

/* Returns the number of characters PRINTed since
 * the last newline
 */
int32_t
print_count(void) {
    return pcount;
}


/*********************************************************************
 *                                                                   *
 * Static execution functions                                        *
 *                                                                   *
 *********************************************************************/

/* Executes an assignment statement */
static int
stmt_exec_assign(struct statement * s) {
    if ( expr_is_builtin(s->e[0]) ) {
        /* PTR# is the only built-in function allowed here */
        return advance_file_ptr(s->e[0], s->e[1]);
    }

    return assign_expr(s->e[0], s->e[1]);
}

/* Executes a BPUT# statement */
static int
stmt_exec_bput(struct statement * s) {
    struct value * c = expr_eval(s->e[0]);
    struct value * b = expr_eval(s->e[1]);
    if ( !c || !b) {
        value_free(c);
        value_free(b);
        return STATUS_ERROR;
    } else if ( !value_is_int(c) || !value_is_int(b) ) {
        value_free(c);
        value_free(b);
        error_set(ERR_TYPE_MISMATCH);
        return ERR_TYPE_MISMATCH;
    }

    const int fd = value_int(c);
    const unsigned char out = value_int(b) % 256;
    value_free(c);
    value_free(b);

    /* Disallow operations on stdin, stdout and stderr */
    if ( fd < 3 ) {
        error_set(ERR_CHANNEL);
        return ERR_CHANNEL;
    }

    int status = write(fd, &out, 1);
    if ( status == -1 ) {
        error_set(ERR_CHANNEL);
        return ERR_CHANNEL;
    } else if ( status == 0 ) {
        error_set(ERR_EOF);
        return ERR_EOF;
    }

    return STATUS_OK;
}

/* Executes a CLEAR statement */
static int
stmt_exec_clear(struct statement * s) {
    symbol_table_clear();
    return STATUS_OK;
}

/* Executes a CLOSE# statement */
static int
stmt_exec_close(struct statement * s) {
    struct value * c = expr_eval(s->e[0]);
    if ( !c ) {
        return STATUS_ERROR;
    } else if ( !value_is_int(c) ) {
        value_free(c);
        error_set(ERR_TYPE_MISMATCH);
        return ERR_TYPE_MISMATCH;
    }

    const int fd = value_int(c);
    value_free(c);

    /* CLOSE# 0 will close all open files */
    if ( fd == 0 ) {
        open_files_close_all();
        return STATUS_OK;
    }

    /* Disallow operations on stdin, stdout and stderr */
    if ( fd < 3 ) {
        error_set(ERR_CHANNEL);
        return ERR_CHANNEL;
    }

    int status = close(fd);
    if ( status == -1 ) {
        error_set(ERR_CHANNEL);
        return ERR_CHANNEL;
    }

    open_file_remove(fd);

    return STATUS_OK;
}

/* Executes a COLOUR statement */
static int
stmt_exec_colour(struct statement * s) {
    struct value * v = expr_eval(s->e[0]);
    if ( !v ) {
        return STATUS_ERROR;
    } else if ( !value_is_int(v) ) {
        value_free(v);
        error_set(ERR_TYPE_MISMATCH);
        return ERR_TYPE_MISMATCH;
    }

    const int colour = value_int(v);
    value_free(v);

#if ENABLE_ANSI_COLOURS
    set_colour_used();

    switch ( colour ) {
        case COLOUR_BLACK:
            printf(ANSI_COLOUR_BLACK);
            break;

        case COLOUR_RED:
            printf(ANSI_COLOUR_RED);
            break;

        case COLOUR_GREEN:
            printf(ANSI_COLOUR_GREEN);
            break;

        case COLOUR_YELLOW:
            printf(ANSI_COLOUR_YELLOW);
            break;

        case COLOUR_BLUE:
            printf(ANSI_COLOUR_BLUE);
            break;

        case COLOUR_MAGENTA:
            printf(ANSI_COLOUR_MAGENTA);
            break;

        case COLOUR_CYAN:
            printf(ANSI_COLOUR_CYAN);
            break;

        case COLOUR_WHITE:
            printf(ANSI_COLOUR_WHITE);
            break;

        case BACKGROUND_COLOUR_BLACK:
            printf(ANSI_BACKGROUND_COLOUR_BLACK);
            break;

        case BACKGROUND_COLOUR_RED:
            printf(ANSI_BACKGROUND_COLOUR_RED);
            break;

        case BACKGROUND_COLOUR_GREEN:
            printf(ANSI_BACKGROUND_COLOUR_GREEN);
            break;

        case BACKGROUND_COLOUR_YELLOW:
            printf(ANSI_BACKGROUND_COLOUR_YELLOW);
            break;

        case BACKGROUND_COLOUR_BLUE:
            printf(ANSI_BACKGROUND_COLOUR_BLUE);
            break;

        case BACKGROUND_COLOUR_MAGENTA:
            printf(ANSI_BACKGROUND_COLOUR_MAGENTA);
            break;

        case BACKGROUND_COLOUR_CYAN:
            printf(ANSI_BACKGROUND_COLOUR_CYAN);
            break;

        case BACKGROUND_COLOUR_WHITE:
            printf(ANSI_BACKGROUND_COLOUR_WHITE);
            break;

        default:
            /* Do nothing */
            break;
    }
#endif

    return STATUS_OK;
}

/* Executes a DIM statement */
static int
stmt_exec_dim(struct statement * s) {
    struct value * vals = NULL;
    struct expr * dim = expr_indices_peek(s->e[0]);
    while ( dim ) {
        struct value * v = expr_eval(dim);
        if ( !v ) {
            value_free(v);
            value_free(vals);
            return STATUS_ERROR;
        } else if ( !value_is_int(v) ) {
            error_set(ERR_TYPE_MISMATCH);
            value_free(v);
            value_free(vals);
            return ERR_TYPE_MISMATCH;
        }
        vals = value_append(vals, v);
        dim = expr_next(dim);
    }

    return symbol_array_dimension(expr_id_peek(s->e[0]), vals);
}

/* Executes an END statement */
static int
stmt_exec_end(struct statement * s) {
    return STATUS_EXIT;
}

/* Executes an ENDPROC statement */
static int
stmt_exec_endproc(struct statement * s) {
    if ( stack_addr_empty(&proc_stack) ) {
        error_set(ERR_NO_PROC);
        return ERR_NO_PROC;
    }

    symbol_table_pop_frame();
    set_pc(stack_addr_pop(&proc_stack));

    return STATUS_OK;
}


/* Executes an FN return statement */
static int
stmt_exec_fn_return(struct statement * s) {
    if ( stack_addr_empty(&fn_stack) ) {
        error_set(ERR_NO_FN);
        return ERR_NO_FN;
    }

    /* Evaluate the return expression */
    struct value * v = expr_eval(s->e[0]);
    if ( !v ) {
        symbol_table_pop_frame();
        set_pc(stack_addr_pop(&fn_stack));

        return STATUS_ERROR;
    }

    /* Push a the return expression onto the runtime stack
     * so it will be available to the evaluation function.
     */
    runtime_stack_push(v);

    /* The evaluation function pushed the stack frame,
     * here pop it and set the program counter to the
     * return address.
     */
    symbol_table_pop_frame();
    set_pc(stack_addr_pop(&fn_stack));

    /* Return STATUS_EXIT because the evaluation function
     * is running inside another statement, and that will
     * handle continuing execution.
     */
    return STATUS_EXIT;
}

/* Executes a FOR statement */
static int
stmt_exec_for(struct statement * s) {
    /* Perform the initializing assignment */
    const int status = statement_execute(s->stmt[0]);
    if ( status != STATUS_OK ) {
        return status;
    }

    /* Push the address of this statement onto the stack. This
     * isn't actually a return or iteration address, as it's the
     * statement following the FOR statement that needs to be
     * executed, but the NEXT command needs access to the loop
     * variable, terminating value and increment, so it needs the
     * address of the FOR statement. The NEXT statement will take
     * care of correctly setting the program counter, so that the
     * initializing assignment above will only be executed once
     * per loop cycle.
     */
    stack_addr_push(&for_stack, s);

    return STATUS_OK;
}

/* Executes a GOSUB statement */
static int
stmt_exec_gosub(struct statement * s) {
    struct value * line = expr_eval(s->e[0]);
    if ( !line ) {
        return STATUS_ERROR;
    }

    if ( !value_is_int(line) ) {
        error_set(ERR_SYNTAX_ERROR);
        value_free(line);
        return ERR_SYNTAX_ERROR;
    }

    struct statement * branch = line_map_find(value_int(line));
    if ( !branch ) {
        error_set(ERR_NO_SUCH_LINE);
        value_free(line);
        return ERR_NO_SUCH_LINE;
    }
    stack_addr_push(&gosub_stack, s->next);
    set_pc(branch);

    value_free(line);

    return STATUS_OK;
}

/* Executes a GOTO statement */
static int
stmt_exec_goto(struct statement * s) {
    struct value * line = expr_eval(s->e[0]);
    if ( !line ) {
        return STATUS_ERROR;
    }

    if ( !value_is_int(line) ) {
        error_set(ERR_SYNTAX_ERROR);
        value_free(line);
        return ERR_SYNTAX_ERROR;
    }

    struct statement * branch = line_map_find(value_int(line));
    if ( !branch ) {
        error_set(ERR_NO_SUCH_LINE);
        value_free(line);
        return ERR_NO_SUCH_LINE;
    }
    set_pc(branch);

    value_free(line);

    return STATUS_OK;
}

/* Executes an IF statement */
static int
stmt_exec_if(struct statement * s) {
    struct value * cond = expr_eval(s->e[0]);
    if ( !cond ) {
        return STATUS_ERROR;
    } else if ( !value_is_numeric(cond) ) {
        error_set(ERR_TYPE_MISMATCH);
        value_free(cond);
        return ERR_TYPE_MISMATCH;
    }

    /* Branch according to condition */
    if ( value_float(cond) ) {
        /* THEN branch if true */
        set_pc(s->stmt[0]);
    } else {
        /* ELSE branch is optional and may be NULL */
        if ( s->stmt[1] ) {
            set_pc(s->stmt[1]);
        }
    } 

    value_free(cond);

    return STATUS_OK;
}

/* Executes an INPUT statement
 * TODO(paul): complete implementation of INPUT statement
 */
static int
stmt_exec_input(struct statement * s) {
    bool output_prompt = false;

    struct print_item * item = s->pl;
    while ( item ) {
        switch ( item->spec ) {
            case PRINT_COMMA:
            case PRINT_SEMICOLON:
                output_prompt = true;
                break;

            case PRINT_EXPR:
                if ( expr_is_variable(item->e) || expr_is_array(item->e) ) {
                    /* Expression is a variable, so get some input */

                    /* Output a prompt if necessary */
                    if ( output_prompt ) {
                        putchar('?');
                        fflush(stdout);
                        output_prompt = false;
                    }

                    /* Read input into a variable. First read a
                     * line from standard input.
                     */
                    char buffer[MAX_LINE_LEN];
                    if ( !fgets(buffer, MAX_LINE_LEN, stdin) ) {
                        if ( errno == EINTR ) {
                            error_set(ERR_ESCAPE);
                            return ERR_ESCAPE;
                        } else {
                            ABORTF("failed to get input: %s", strerror(errno));
                        }
                    }

                    /* Remove the trailing newline, if present */
                    size_t buflen = strlen(buffer);
                    if ( buflen > 0 && buffer[buflen-1] == '\n' ) {
                        buffer[buflen-1] = '\0';
                        buflen -= 1;
                    } else {
                        /* No newline means the input exceeded one line,
                         * which is the maximum length of a string
                         */
                        error_set(ERR_STRING_TOO_LONG);
                        return STATUS_ERROR;
                    }

                    /* Remove leading spaces if this is not
                     * an INPUT LINE operation
                     */
                    if ( !s->v ) {
                        size_t i = 0;
                        while ( isspace(buffer[i]) ) {
                            i++;
                        }

                        if ( i != 0 ) {
                            memmove(buffer, buffer+i, buflen-i+1);
                        }
                    }

                    /* Parse input depending on variable type */
                    const char * varname = expr_id_peek(item->e);
                    struct expr * input;
                    if ( variable_name_is_string(varname) ) {
                        /* String variable, so store the whole line */
                        input = expr_string_new(buffer);
                    } else if ( variable_name_is_resident(varname)
                            || variable_name_is_integer(varname) ) {
                        /* Resident integer variable, so read an integer,
                         * discarding any trailing input
                         */
                        char * endptr;
                        errno = 0;
                        const long n = strtol(buffer, &endptr, 10);
                        if ( errno == ERANGE || endptr == buffer || n > INT_MAX ) {
                            /* Any errors result in a value of zero */
                            input = expr_int_new(0);
                        } else {
                            input = expr_int_new(n);
                        }
                    } else {
                        /* Numeric variable, so read a double,
                         * discarding any trailing input
                         */
                        char * endptr;
                        errno = 0;
                        const double d = strtod(buffer, &endptr);
                        if ( errno == ERANGE || endptr == buffer ) {
                            /* Any errors result in a value of zero */
                            input = expr_int_new(0);
                        } else {
                            input = expr_float_new(d);
                        }
                    }

                    /* Assign the value to the variable */
                    const int status = assign_expr(item->e, input);
                    expr_free(input);
                    if ( status != STATUS_OK ) {
                        return STATUS_ERROR;
                    }
                } else {
                    /* Expression is not a variable, so output some text */
                    struct value * v = expr_eval(item->e);
                    if ( !v ) {
                        return STATUS_ERROR;
                    }

                    printf("%s", value_string_peek(v));
                    value_free(v);
                    fflush(stdout);

                    output_prompt = false;
                }

                break;

            default:
                ABORTF("unexpected print item type: %d", item->spec);
        }
        item = item->next;
    }

    return STATUS_OK;
}

/* Executes a INPUT# statement
 */
static int
stmt_exec_inputf(struct statement * s) {
    /* Retrieve the file descriptor */
    struct value * c = expr_eval(s->e[0]);
    if ( !c ) {
        return STATUS_ERROR;
    } else if ( !value_is_int(c) ) {
        value_free(c);
        error_set(ERR_TYPE_MISMATCH);
        return ERR_TYPE_MISMATCH;
    }

    const int fd = value_int(c);
    value_free(c);

    /* Disallow operations on stdin, stdout and stderr */
    if ( fd < 3 ) {
        error_set(ERR_CHANNEL);
        return ERR_CHANNEL;
    }

    char buffer[BUFFER_SIZE];

    /* Loop through the variables */
    struct expr * var = s->e[1];
    while ( var ) {
        /* Read the leading byte */
        unsigned char t;
        if ( read(fd, &t, 1) == -1 ) {
            error_set(ERR_CHANNEL);
            return ERR_CHANNEL;
        }

        struct value * v;

        switch ( t ) {
            case 0x40:
                /* Integer */
                if ( read(fd, buffer, sizeof(int32_t)) != sizeof(int32_t) ) {
                    error_set(ERR_CHANNEL);
                    return ERR_CHANNEL;
                }

                /* Integers are stored with most significant byte first */
                const int32_t n = ((uint8_t)buffer[0] << 24) |
                    ((uint8_t)buffer[1] << 16) |
                    ((uint8_t)buffer[2] << 8) |
                    (uint8_t)buffer[3];
                v = value_int_new(n);

                break;

            case 0xff:
                /* Float */
                if ( read(fd, buffer, sizeof(double)) != sizeof(double) ) {
                    error_set(ERR_CHANNEL);
                    return ERR_CHANNEL;
                }

                double d;
                memcpy(&d, buffer, sizeof(d));
                v = value_float_new(d);

                break;

            case 0x00:
                /* String, so read the next byte containing the length.
                 * We won't use the original t again, so it's safe to
                 * reuse it.
                 */
                if ( read(fd, &t, 1) != 1 ) {
                    error_set(ERR_CHANNEL);
                    return ERR_CHANNEL;
                }

                t = (uint8_t) t; /* In case CHAR_BIT > 8 */

                /* Read the string itself, and null-terminate it */
                if ( read(fd, &buffer, t) != t ) {
                    error_set(ERR_CHANNEL);
                    return ERR_CHANNEL;
                }
                buffer[t] = '\0';

                v = value_string_new(buffer);
                break;

            default:
                error_set(ERR_CHANNEL);
                return ERR_CHANNEL;
        }

        /* Increment the file pointer */
        open_file_increment_ptr(fd);

        /* Assign the value to the variable */
        int status = assign_value(var, v);
        if ( status != STATUS_OK ) {
            return STATUS_ERROR;
        }

        value_free(v);
        var = expr_next(var);
    }

    return STATUS_OK;
}

/* Executes a LOCAL statement */
static int
stmt_exec_local(struct statement * s) {
    /* Return an error if LOCAL appears outside a
     * procedure or function
     */
    if ( stack_addr_empty(&fn_stack) && stack_addr_empty(&proc_stack) ) {
        error_set(ERR_NOT_LOCAL);
        return ERR_NOT_LOCAL;
    }

    /* Loop through the list of variables, and assign an
     * appropriate zero value to each of them.
     */
    struct expr * var = s->e[0];
    while ( var ) {
        symbol_variable_assign_local(expr_id_peek(var), NULL);
        var = expr_next(var);
    }
        
    return STATUS_OK;
}

/* Executes a NEXT statement */
static int
stmt_exec_next(struct statement * s) {
    /* Get the matching for statement from the stack */
    struct statement * branch = for_stack_peek(s->e[0]);
    if ( !branch ) {
        if ( !s->e[0] ) {
            error_set(ERR_NO_FOR);
            return ERR_NO_FOR;
        } else {
            error_set(ERR_CANT_MATCH_FOR);
            return ERR_CANT_MATCH_FOR;
        }
    }

    /* Evaluate the loop terminating value and increment.
     * Note that if the loop increment is zero, we'll loop
     * forever, but BBC BASIC doesn't check for this, so we
     * won't either. We will abort, though, as we're not
     * entirely without a heart.
     */
    struct expr * var = branch->stmt[0]->e[0];
    struct value * term = expr_eval(branch->e[0]);
    if ( !term ) {
        return STATUS_ERROR;
    }

    struct value * step = expr_eval(branch->e[1]);
    if ( !term ) {
        value_free(term);
        return STATUS_ERROR;
    }

    if ( !value_is_numeric(term) || !value_is_numeric(step) ) {
        error_set(ERR_TYPE_MISMATCH);
        value_free(term);
        value_free(step);
        return ERR_TYPE_MISMATCH;
    } else if ( value_is_zero(step) ) {
        ABORT("loop increment is zero");
    }

    /* Add the increment to the loop variable */
    const int status = increment_var(s, var, step);
    if ( status != STATUS_OK ) {
        /* BBC BASIC II returns a 'FOR variable' error when
         * the variable in a FOR loop is not a numeric
         * variable
         */
        if ( (status == ERR_TYPE_MISMATCH) ||
                (error_code() == ERR_TYPE_MISMATCH) ) {
            error_set(ERR_FOR_VARIABLE);
        }
        value_free(term);
        value_free(step);
        return ERR_FOR_VARIABLE;
    }

    /* Compare the loop variable with the terminating value */
    struct expr * ecmp;
    if ( value_int(step) > 0 ) {
        ecmp = expr_op_gt_new(expr_variable_new(expr_id_peek(var)), expr_constant_new(term));
    } else {
        ecmp = expr_op_lt_new(expr_variable_new(expr_id_peek(var)), expr_constant_new(term));
    }

    struct value * cmp = expr_eval(ecmp);
    if ( !cmp ) {
        expr_free(ecmp);
        value_free(term);
        value_free(step);
        return STATUS_ERROR;
    }

    if ( value_is_zero(cmp) ) {
        /* Comparison is FALSE, so loop again and set the
         * program counter to the instruction following
         * the FOR statement.
         */
        set_pc(branch->next);
    } else {
        /* Comparison is TRUE, so pop the FOR statement
         * from the stack and continue normal execution.
         */
        stack_addr_pop(&for_stack);
    }

    expr_free(ecmp);
    value_free(cmp);
    value_free(term);
    value_free(step);

    return STATUS_OK;
}

/* Executes a null statement */
static int
stmt_exec_null(struct statement * s) {
    return STATUS_OK;
}

/* Executes an ON ERROR statement */
static int
stmt_exec_on_error(struct statement * s) {
    if ( !s->stmt[0] ) {
        /* ON ERROR OFF was called, so clear the error statement */
        error_stmt_clear();
    } else {
        /* ON ERROR <statement> was called, so set the error statement */
        s->stmt[0]->line_number = s->line_number;
        error_stmt_set(s->stmt[0], s->next);
    }

    return STATUS_OK;
}

/* Executes an ON GOTO or ON GOSUB statement */
static int
stmt_exec_on(struct statement * s, bool gosub) {
    /* Evaluate the branch value */
    struct value * val = expr_eval(s->e[0]);
    if ( !val ) {
        return STATUS_ERROR;
    } else if ( !value_is_numeric(val) ) {
        error_set(ERR_TYPE_MISMATCH);
        value_free(val);
        return ERR_TYPE_MISMATCH;
    }

    const int branch = value_int(val);
    value_free(val);

    if ( branch < 1 ) {
        /* Execute the optional ELSE, if present */
        if ( s->stmt[0] ) {
            if ( gosub ) {
                stack_addr_push(&gosub_stack, s->next);
            }
            set_pc(s->stmt[0]);

            return STATUS_OK;
        }

        /* Otherwise return an error */
        error_set(ERR_ON_RANGE);
        return ERR_ON_RANGE;
    }

    /* Cycle through the target lines */
    struct expr * line_expr = s->e[1];
    int index = 1;
    while ( line_expr ) {
        /* Skip based on branch value */
        if ( index != branch ) {
            line_expr = expr_next(line_expr);
            index++;
            continue;
        }

        /* Evaluate the target line */
        struct value * vline = expr_eval(line_expr);
        if ( !vline ) {
            return STATUS_ERROR;
        } else if ( !value_is_numeric(vline) ) {
            error_set(ERR_TYPE_MISMATCH);
            value_free(vline);
            return ERR_TYPE_MISMATCH;
        }

        const int line = value_int(vline);
        value_free(vline);

        /* Branch to the target line */
        struct statement * branch_stmt = line_map_find(line);
        if ( !branch_stmt ) {
            error_set(ERR_NO_SUCH_LINE);
            return ERR_NO_SUCH_LINE;
        }
        if ( gosub ) {
            stack_addr_push(&gosub_stack, s->next);
        }
        set_pc(branch_stmt);

        return STATUS_OK;
    }

    /* We didn't find a match, so execute the optional
     * ELSE, if present
     */
    if ( s->stmt[0] ) {
        if ( gosub ) {
            stack_addr_push(&gosub_stack, s->next);
        }
        set_pc(s->stmt[0]);

        return STATUS_OK;
    }

    /* Otherwise return an error */
    error_set(ERR_ON_RANGE);

    return ERR_ON_RANGE;
}

/* Executes an ON GOSUB statement */
static int
stmt_exec_on_gosub(struct statement * s) {
    return stmt_exec_on(s, true);
}

/* Executes an ON GOTO statement */
static int
stmt_exec_on_goto(struct statement * s) {
    return stmt_exec_on(s, false);
}

/* Executes a PRINT statement
 */
static int
stmt_exec_print(struct statement * s) {
    struct print_item * item = s->pl;
    if ( !item ) {
        /* Print a blank line if no print list was specified */
        putchar('\n');
        pcount = 0;
        return STATUS_OK;
    }

    const int width = format_width();       /* Field width */
    enum print_specifier spec = PRINT_EXPR; /* Previous specifier */

    while ( item ) {
        switch ( item->spec ) {
            case PRINT_APOSTROPHE:
                /* A new line can be forced at any stage in the print
                 * list by inserting an apostrophe.
                 */
                putchar('\n');
                pcount = 0;
                spec = PRINT_APOSTROPHE;
                break;

            case PRINT_COMMA:
                spec = PRINT_COMMA;
                break;

            case PRINT_SEMICOLON:
                spec = PRINT_SEMICOLON;
                break;

            case PRINT_EXPR:
                if ( spec == PRINT_COMMA ) {
                    while ( (pcount % width) != 0 ) {
                        /* Output enough spaces to ensure that this
                         * item will be printed in the field after
                         * the previous item
                         */
                        putchar(' ');
                        pcount++;
                    }
                }

                struct value * v = expr_eval(item->e);
                if ( !v ) {
                    return STATUS_ERROR;
                }

                char * out;
                if ( spec == PRINT_SEMICOLON ) {
                    /* A semi-colon after an item in the print list
                     * will cause the next item to be printed on the
                     * same line and immediately following the previous
                     * item, so turn off field width in the case.
                     */
                    out = value_to_string(v, false);
                } else {
                    out = value_to_string(v, true);
                }

                printf("%s", out);
                pcount += strlen(out);
                free(out);
                value_free(v);

                /* Reset the previous specifier value */
                spec = PRINT_EXPR;

                break;

            default:
                ABORTF("unrecognized print item type: %d", item->spec);
        }
        item = item->next;
    }

    if ( spec != PRINT_SEMICOLON ) {
        putchar('\n');
        pcount = 0;
    } else {
        fflush(stdout);
    }

    return STATUS_OK;
}

/* Executes a PRINT# statement
 */
static int
stmt_exec_printf(struct statement * s) {
    /* Retrieve the file descriptor */
    struct value * c = expr_eval(s->e[0]);
    if ( !c ) {
        return STATUS_ERROR;
    } else if ( !value_is_int(c) ) {
        value_free(c);
        error_set(ERR_TYPE_MISMATCH);
        return ERR_TYPE_MISMATCH;
    }

    const int fd = value_int(c);
    value_free(c);

    /* Disallow operations on stdin, stdout and stderr */
    if ( fd < 3 ) {
        error_set(ERR_CHANNEL);
        return ERR_CHANNEL;
    }

    char buffer[BUFFER_SIZE];

    /* Loop through the expressions */
    struct expr * e = s->e[1];
    while ( e ) {
        struct value * v = expr_eval(e);
        if ( !v ) {
            return STATUS_ERROR;
        }

        /* Write according to type */
        if ( value_is_int(v) ) {
            const int32_t n = value_int(v);
            value_free(v);

            /* Integers are represented by 0x40 followed by the four
             * value bytes, with most significant byte first. Note that
             * BBC Basic II specifically requires integers to be stored
             * in two's complement, and while C in general does not, it
             * does require the signed intN_t integer types to have a
             * two's complement representation (see C17 7.2.0.1.1.3),
             * so the behavior will be correct on a conforming
             * implementation.
             */
            buffer[0] = 0x40;
            buffer[1] = (((uint32_t) n) & 0xFF000000) >> 24;
            buffer[2] = (((uint32_t) n) & 0xFF0000) >> 16;
            buffer[3] = (((uint32_t) n) & 0xFF00) >> 8;
            buffer[4] = ((uint32_t) n) & 0xFF;

            if ( write(fd, buffer, 5) == -1 ) {
                error_set(ERR_CHANNEL);
                return ERR_CHANNEL;
            }
        } else if ( value_is_float(v) ) {
            const double d = value_float(v);
            value_free(v);

            /* Real numbers are represented by 0xff, followed by the
             * double value. Note that this is inconsistent with BBC
             * BASIC II on the BBC Micro, where it is stored as 0xff
             * followed by four bytes of mantissa and one byte of
             * exponent.
             */
            buffer[0] = 0xff;
            memcpy(&buffer[1], &d, sizeof(d));

            if ( write(fd, buffer, 1 + sizeof(d)) == -1 ) {
                error_set(ERR_CHANNEL);
                return ERR_CHANNEL;
            }
        } else if ( value_is_string(v) ) {
            const size_t sl = strlen(value_string_peek(v));
            const size_t nb = sl > 255 ? 255 : sl;

            /* Strings are represented by 0x00, followed by a single
             * octet containing the string length, followed by the
             * string bytes themselves. The maximum length of a string
             * is 255.
             */
            buffer[0] = 0x00;
            buffer[1] = nb;
            strncpy(&buffer[2], value_string_peek(v), nb);
            value_free(v);

            if ( write(fd, buffer, 2 + nb) == -1 ) {
                error_set(ERR_CHANNEL);
                return ERR_CHANNEL;
            }
        } else {
            ABORT("unexpected value type");
        }

        /* Increment the file pointer */
        open_file_increment_ptr(fd);

        e = expr_next(e);
    }

    return STATUS_OK;
}

/* Executes a PROC statement */
static int
stmt_exec_proc(struct statement * s) {
    /* Retrieve the procedure address */
    struct statement * branch = symbol_proc_call(value_string_peek(s->v));
    if ( !branch ) {
        error_set(ERR_NO_SUCH_FN_PROC);
        return ERR_NO_SUCH_FN_PROC;
    }

    const enum basic_error err = pass_arguments(branch->e[0], s->e[0]);
    if ( err != ERR_NO_ERROR ) {
        return err;
    }

    /* Push the return address and branch */
    stack_addr_push(&proc_stack, s->next);
    set_pc(branch);

    return STATUS_OK;
}

/* Executes a READ statement */
static int
stmt_exec_read(struct statement * s) {
    struct expr * var = s->e[0];
    while ( var ) {
        if ( !data_ptr ) {
            error_set(ERR_OUT_OF_DATA);
            return ERR_OUT_OF_DATA;
        }

        const int status = assign_value(var, data_ptr);
        if ( status != STATUS_OK ) {
            return STATUS_ERROR;
        }

        var = expr_next(var);
        data_ptr = value_next(data_ptr);
    }
        
    return STATUS_OK;
}

/* Executes a REPEAT statement */
static int
stmt_exec_repeat(struct statement * s) {
    /* If the return address stack is empty, or if the address
     * at the top of the stack is not the address of this
     * statement, then we're not currently in a loop with this
     * REPEAT statement, so we need to push the address onto
     * the stack. Otherwise, we just keep going and execute any
     * attached statement again. It is possible for this logic
     * to fail if the user is abusing GOTO and jumping in and
     * out of loops, but they can take their chances if they're
     * doing that.
     */
    if ( stack_addr_empty(&repeat_stack) ||
            (stack_addr_peek(&repeat_stack) != s) ) {
        stack_addr_push(&repeat_stack, s);
    }

    /* Execute the optional statement, if present */
    if ( s->stmt[0] ) {
        set_pc(s->stmt[0]);
    }

    return STATUS_OK;
}

/* Executes a REPORT statement */
static int
stmt_exec_report(struct statement * s) {
    const char * msg = error_string(error_last_code());
    if ( !msg ) {
        printf("\n(C)1982 Acorn\n");
        pcount = 0;
    } else {
        printf("\n%s", msg);
        fflush(stdout);
        pcount = strlen(msg);
    }

    return STATUS_OK;
}

/* Executes a RESTORE statement */
static int
stmt_exec_restore(struct statement * s) {
    /* Reset to head of data items list if no line provided */
    if ( !s->e[0] ) {
        reset_data_pointer();

        return STATUS_OK;
    }

    /* Evaluate line number */
    struct value * v = expr_eval(s->e[0]);
    if ( !v ) {
        return STATUS_ERROR;
    } else if ( !value_is_int(v) ) {
        error_set(ERR_TYPE_MISMATCH);
        value_free(v);
        return ERR_TYPE_MISMATCH;
    }

    const int line = value_int(v);
    value_free(v);

    /* Look for data pointer in data map.
     * TODO(paul): BBC BASIC II does not require the line number
     * to refer to an actual data statement, and will restore
     * the data pointer to the next line containing a DATA
     * statement. Need to implement this behavior.
     */
    struct value * data = data_map_find(line);
    if ( !data ) {
        error_set(ERR_NO_SUCH_LINE);
        return ERR_NO_SUCH_LINE;
    }

    /* Set the data pointer */
    data_ptr = data;

    return STATUS_OK;
}

/* Executes a RETURN statement */
static int
stmt_exec_return(struct statement * s) {
    if ( stack_addr_empty(&gosub_stack) ) {
        error_set(ERR_NO_GOSUB);
        return STATUS_ERROR;
    }
    set_pc(stack_addr_pop(&gosub_stack));

    return STATUS_OK;
}

/* Executes a STOP statement */
static int
stmt_exec_stop(struct statement * s) {
    fprintf(stderr, "STOP at line %d\n", s->line_number);
    return STATUS_EXIT;
}

/* Executes a TRACE statement */
static int
stmt_exec_trace(struct statement * s) {
    /* Turn TRACEing on or off */
    if ( value_int(s->v) == TRACE_ON ) {
        trace_on = true;
        trace_last_line = s->line_number;
    } else {
        trace_on = false;
        trace_last_line = 0;
    }

    /* Set the TRACEing line threshold */
    struct value * line = expr_eval(s->e[0]);
    if ( !line ) {
        return STATUS_ERROR;
    } else if ( !value_is_int(line) ) {
        /* Passing a float to TRACE should generate a
         * syntax error.
         */
        error_set(ERR_SYNTAX_ERROR);
        value_free(line);
        return ERR_SYNTAX_ERROR;
    }

    trace_threshold = value_int(line);

    value_free(line);

    return STATUS_OK;
}

/* Executes an UNTIL statement */
static int
stmt_exec_until(struct statement * s) {
    struct value * cond = expr_eval(s->e[0]);
    if ( !cond ) {
        return STATUS_ERROR;
    } else if ( !value_is_numeric(cond) ) {
        error_set(ERR_TYPE_MISMATCH);
        value_free(cond);
        return ERR_TYPE_MISMATCH;
    }

    if ( stack_addr_empty(&repeat_stack) ) {
        error_set(ERR_NO_REPEAT);
        value_free(cond);
        return ERR_NO_REPEAT;
    }

    /* Branch if the expression is FALSE (0) but pop the return
     * address and continue normal execution if it is TRUE.
     */
    if ( value_float(cond) ) {
        stack_addr_pop(&repeat_stack);
    } else {
        set_pc(stack_addr_peek(&repeat_stack));
    }

    value_free(cond);

    return STATUS_OK;
}


/*********************************************************************
 *                                                                   *
 * Static helper functions                                           *
 *                                                                   *
 *********************************************************************/

/* Advances a file pointer */
static int
advance_file_ptr(struct expr * c, struct expr * e) {
    struct value * cval = expr_eval(expr_sub(c, 0));
    struct value * eval = expr_eval(e);
    if ( !cval || !eval ) {
        value_free(cval);
        value_free(eval);
        return STATUS_ERROR;
    } else if ( !value_is_numeric(cval) || !value_is_numeric(eval) ) {
        error_set(ERR_TYPE_MISMATCH);
        value_free(cval);
        value_free(eval);
        return ERR_TYPE_MISMATCH;
    }

    const int fd = value_int(cval);
    const int want = value_int(eval);
    value_free(cval);
    value_free(eval);

    /* Disallow operations on stdin, stdout and stderr */
    if ( fd < 3 ) {
        error_set(ERR_CHANNEL);
        return ERR_CHANNEL;
    }

    /* Set file position to beginning */
    if ( lseek(fd, 0, SEEK_SET) == -1 ) {
        error_set(ERR_CHANNEL);
        return ERR_CHANNEL;
    }
    open_file_set_ptr(fd, 0);

    /* Advance to desired pointer */
    char buffer[BUFFER_SIZE];
    while ( open_file_get_ptr(fd) < want ) {
        unsigned char b;
        if ( read(fd, &b, 1) == -1 ) {
            error_set(ERR_CHANNEL);
            return ERR_CHANNEL;
        }

        switch ( b ) {
            case 0x40:
                /* Integer */
                if ( read(fd, buffer, 4) != 4 ) {
                    error_set(ERR_CHANNEL);
                    return ERR_CHANNEL;
                }

                break;

            case 0xff:
                /* Float */
                if ( read(fd, buffer, sizeof(double)) != sizeof(double) ) {
                    error_set(ERR_CHANNEL);
                    return ERR_CHANNEL;
                }

                break;

            case 0x00:
                /* String */
                if ( read(fd, &b, 1) != 1 ) {
                    error_set(ERR_CHANNEL);
                    return ERR_CHANNEL;
                }

                if ( read(fd, buffer, (uint8_t) b) != (uint8_t) b ) {
                    error_set(ERR_CHANNEL);
                    return ERR_CHANNEL;
                }

                break;

            default:
                error_set(ERR_CHANNEL);
                return ERR_CHANNEL;
        }

        open_file_increment_ptr(fd);
    }

    return STATUS_OK;
}

/* Assigns e to var */
static int
assign_expr(struct expr * var, struct expr * e) {
    struct value * v = expr_eval(e);
    if ( !v ) {
        return STATUS_ERROR;
    }

    const int status = assign_value(var, v);
    value_free(v);

    return status;
}

/* Assigns v to var */
static int
assign_value(struct expr * var, struct value * v) {
    int status;
    if ( expr_is_variable(var) ) {
        status = symbol_variable_assign(expr_id_peek(var), v);
    } else if ( expr_is_array(var) ) {
        struct value * indices = eval_array_indices(var);
        if ( !indices ) {
            return STATUS_ERROR;
        }

        status = symbol_array_assign(expr_id_peek(var), indices, v);
        value_free(indices);
    } else {
        ABORT("unexpected expression type");
    }

    return status;
}

/* Creates and initializes a new statement */
static struct statement *
create(enum statement_type type) {
    struct statement * stmt = x_malloc(sizeof *stmt);
    stmt->type = type;
    stmt->v = NULL;
    stmt->pl = NULL;
    stmt->next = NULL;
    stmt->exec = NULL;

    for ( size_t i = 0; i < STMT_NUM_EXPRS; i++ ) {
        stmt->e[i] = NULL;
    }

    for ( size_t i = 0; i < STMT_NUM_STMTS; i++ ) {
        stmt->stmt[i] = NULL;
    }

    return stmt;
}

/* Evaluates a set of array indices */
static struct value *
eval_array_indices(struct expr * var) {
    struct value * vals = NULL;
    struct expr * dims = expr_indices_peek(var);
    while ( dims ) {
        struct value * index = expr_eval(dims);
        if ( !index ) {
            value_free(vals);
            return NULL;
        }

        vals = value_append(vals, index);
        dims = expr_next(dims);
    }

    return vals;
}

/* Peeks at the FOR loop statement on the stack for the specified
 * variable expression. If the expression is NULL, the top of the
 * stack will be peeked. If the expression is not NULL and the
 * FOR loop statement corresponding to the expression is not at
 * the top of the stack, entries will be popped off the stack
 * until the desired FOR loop statement is located. If the stack
 * is empty or becomes empty before an appropriate FOR loop
 * statement is found, NULL is returned.
 */
static struct statement *
for_stack_peek(struct expr * e) {
    /* Return error if stack is empty */
    if ( stack_addr_empty(&for_stack) ) {
        return NULL;
    }

    /* If no expression was specified, peek at the top of the stack */
    if ( !e ) {
        return stack_addr_peek(&for_stack);
    }

    /* An expression was specified, so search for it */
    const char * id = expr_id_peek(e);
    struct statement * match;
    while ( (match = stack_addr_peek(&for_stack)) ) {
        char * found = expr_id_peek(match->stmt[0]->e[0]);
        if ( !strcmp(id, found) ) {
            /* We found it */
            break;
        }

        /* We didn't find it, so pop from the stack and keep looking */
        stack_addr_pop(&for_stack);
        if ( stack_addr_empty(&for_stack) ) {
            /* Stack is empty, so we're not going to find it */
            return NULL;
        }
    }

    return match;
}

/* Internal recursive implementation of statement_free */
static void
free_internal(struct statement * stmt, struct addr_set * set) {
    while ( stmt != NULL ) {
        /* A list of statements can form a graph rather than a tree, so
         * ensure we don't attempt to free any statement more than once
         */
        if ( addr_set_is_member(set, stmt) ) {
            return;
        }
        addr_set_add(set, stmt);

        struct statement * tmp = stmt->next;
        
        /* Reset any value */
        value_free(stmt->v);

        /* Free any expressions */
        for ( size_t i = 0; i < STMT_NUM_EXPRS; i++ ) {
            expr_free(stmt->e[i]);
        }

        /* Free any sub-statements or sub-statement branches */
        for ( size_t i = 0; i < STMT_NUM_STMTS; i++ ) {
            free_internal(stmt->stmt[i], set);
        }

        /* Free any print list */
        if ( stmt->pl ) {
            print_list_free(stmt->pl);
        }

        free(stmt);
        stmt = tmp;
    }
}

/* Increments a variable without having to make a copy
 * of the variable or the increment
 */
static int
increment_var(struct statement * parent, struct expr * var, struct value * inc) {
    /* Create temporary addition expression */
    struct expr * tvar1 = expr_variable_new(expr_id_peek(var));
    struct expr * tinc = expr_constant_new(inc);
    struct expr * add = expr_op_add_new(tvar1, tinc);

    const int status = assign_expr(var, add);
    expr_free(add);

    return status;
}

/* Allocates a new print item */
static struct print_item *
print_item_new(enum print_specifier spec, struct expr * e) {
    struct print_item * item = x_malloc(sizeof *item);
    item->spec = spec;
    item->e = e;
    item->next = NULL;
    return item;
}

/* Recursively add a parent line number to a statement and
 * any sub-statements it has.
 */
static void
update_line_numbers(struct statement *s, const int line_number) {
    while ( s ) {
        s->line_number = line_number;
        for ( size_t i = 0; i < STMT_NUM_STMTS; i++ ) {
            if ( !s->stmt[i] ) {
                break;
            }

            update_line_numbers(s->stmt[i], line_number);
        }
        s = s->next;
    }
}
