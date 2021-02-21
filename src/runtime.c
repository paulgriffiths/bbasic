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

/* Functionality for running programs */

#include "internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "runtime.h"
#include "statements.h"
#include "stack_addr.h"
#include "symbols.h"
#include "line_map.h"
#include "util.h"
#include "options.h"
#include "file_set.h"
#include "colours.h"

/* List of program lines */
static struct line {
    int number;
    struct statement * stmt;
    struct line * next;
} * lines;

/* List of program statements */
static struct statement * stmts;

/* Currently executing line */
static int current_line;

/* Pointer to next statement to execute */
static struct statement * program_counter;

/* Pointer to ON ERROR handling statment, if any */
static struct statement * error_stmt;

/* Lines and codes of current and last-reported errors */
static struct error_register {
    int line;
    enum basic_error code;
    int last_line;
    enum basic_error last_code;
} error_register = {
   .line = 0,
   .code = ERR_NO_ERROR,
   .last_line = 0,
   .last_code = ERR_NO_ERROR,
};

/* Function return value stack */
static struct stack_addr return_stack;

/* Open files list */
static struct file_set files_list;

#ifdef ENABLE_ANSI_COLOURS
/* "Colours used" flag */
static bool colours_used;

/* Reset colours exit handler */
static void reset_colours(void);
#endif

/* Static function declarations */
static bool status_error(const int status);

/* Interrupt flag */
volatile sig_atomic_t interrupt;

/* Builds a list of program statements from a list of program lines */
static void
build_statements(void) {
    struct line * line = lines;
    struct statement * tail = NULL;

    while ( line ) {
        struct statement * stmt = line->stmt;
        struct statement * next = stmt->next;

        while ( stmt ) {
            /* Update the line number for each statement, so it's
             * available to the statement execution routines
             * */
            stmt->line_number = line->number;

            /* Add or append the statement */
            if ( !tail ) {
                stmts = stmt;
                tail = stmts;
            } else {
                tail->next = stmt;
                tail = stmt;
            }

            /* Perform specific processing for some statements */
            switch ( stmt->type ) {
                case STATEMENT_DEF_FN:
                case STATEMENT_DEF_PROC:
                    symbol_proc_define(value_string_peek(stmt->v), stmt);
                    break;

                default:
                    /* No specific processing */
                    break;
            }

            /* Advance pointers */
            stmt = stmt->next;
            next = stmt ? stmt->next : NULL;

            /* Some statements recursively contain a list of one or
             * more statements (e.g. IF, REPEAT). These statements
             * need the address of (from what here is) the next
             * statement, so that program execution can continue
             * normally after those lists of statement complete.
             */
            statement_fixup(tail,
                    ((!stmt && line->next) ? line->next->stmt : stmt));

        }
        line = line->next;
    }
}

#if ENABLE_ANSI_COLOURS
/* Reset colours exit handler */
static void
reset_colours(void) {
    if ( colours_used ) {
        printf(ANSI_COLOUR_RESET);
        fflush(stdout);
    }
}
#endif

/* Pops an expression from the runtime stack */
struct value *
runtime_stack_pop(void) {
    if ( stack_addr_empty(&return_stack) ) {
        ABORT("runtime stack empty");
        return NULL;
    }

    return (struct value *)stack_addr_pop(&return_stack);
}

/* Pushes an expression onto the runtime stack */
void
runtime_stack_push(struct value * v) {
    stack_addr_push(&return_stack, (struct statement *) v);
}

/* Runs a sequence of statements */
int
run_statements(struct statement * s) {
    int status;

    set_pc(s);
    struct statement * current = program_counter;
    while ( (current = pc()) ) {
        /* The statement may alter the program counter, so
         * set the default before we execute it, not after
         */
        set_pc(current->next);
        
        /* Store current line number for error reporting */
        current_line = current->line_number;

        status = statement_execute(current);
        if ( error_stmt && ((status == STATUS_ERROR) || (status > 0)
                    || error_is_set()) ) {
            /* ON ERROR handling is active, so set the program
             * counter to the error handling statement, and clear
             * the current (but not the last reported) error status.
             * Note that error code of zero cannot be caught.
             */
            set_pc(error_stmt);
            error_clear();
        } else if ( interrupt ) {
            /* A signal was received, probably SIGINT, so break */
            error_set(ERR_ESCAPE);
            break;
        } else if ( status != STATUS_OK ) {
            /* Normal program termination or an error */
            break;
        }
    }

    return status;
}

/* Runs a program */
int
program_run(void) {

    /* Set up */
    error_clear();
    build_statements();
    reset_data_pointer();

#if ENABLE_ANSI_COLOURS
    x_atexit(reset_colours);
#endif

    int status = run_statements(stmts);

    /* Cleanup */
    runtime_free();

    // Return the status code on error.
    if ( status_error(status) || error_is_set() ) {
        if ( !status_error(status) ) {
            status = error_code();
        }
        error_output();
        return status;
    }

    return STATUS_OK;
}

/* Frees resources used by the runtime */
void
runtime_free(void) {
    /* Free program statements */
    error_stmt_clear();
    statement_free(stmts);
    stmts = NULL;

    /* Free program lines */
    struct line * line = lines;
    while ( line ) {
        struct line * tmp = line->next;
        free(line);
        line = tmp;
    }
    lines = NULL;

    /* Free other resources */
    stack_addr_free(&return_stack);
    open_files_close_all();
    file_set_free(&files_list);
    symbol_table_free();
    line_map_free();
    statements_cleanup();
}


/*********************************************************************
 *                                                                   *
 * Program line functions                                            *
 *                                                                   *
 *********************************************************************/

/* Adds, inserts, or replaces a line in the program */
int
line_add(const int number, struct statement * stmt) {
    struct line * new_line = x_malloc(sizeof *new_line);
    new_line->number = number;
    new_line->stmt = stmt;
    new_line->next = NULL;

    /* Store the address of the first statement of the line in
     * a lookup table, for faster GOTO and GOSUB branching
     */
    int status;
    if ( (status = line_map_add(number, stmt)) != STATUS_OK ) {
        runtime_free();
        free(new_line);
        return status;
    }

    /* If the program is empty, just add this one line and return */
    if ( lines == NULL ) {
        lines = new_line;
        return STATUS_OK;
    }

    struct line * current = lines;
    struct line * previous = NULL;

    /* Insert or append the line at the appropriate place */
    while ( current != NULL ) {
        if ( number < current->number ) {
            new_line->next = current;
            if ( previous == NULL ) {
                lines = new_line;
            } else {
                previous->next = new_line;
            }

            return STATUS_OK;
        }

        previous = current;
        current = current->next;
    }

    /* No lower-numbered lines exist and the program is not empty, so
     * append the new line.
     */
    previous->next = new_line;

    return STATUS_OK;
}

/*********************************************************************
 *                                                                   *
 * Program counter functions                                         *
 *                                                                   *
 *********************************************************************/

/* Returns the program counter */
struct statement *
pc(void) {
    return program_counter;
}

/* Sets the program counter */
void
set_pc(struct statement * stmt) {
    program_counter = stmt;
}


/*********************************************************************
 *                                                                   *
 * Open file functions                                               *
 *                                                                   *
 *********************************************************************/

/* Adds a file to the open files list */
void
open_file_add(const int fd) {
    file_set_add(&files_list, fd);
}

/* Closes all open files */
void
open_files_close_all(void) {
    while ( !file_set_empty(&files_list) ) {
        if ( close(file_set_pop(&files_list)) == -1 ) {
            ABORTF("close failed: %s", strerror(errno));
        }
    }
}

/* Gets the file pointer for an open file */
int
open_file_get_ptr(const int fd) {
    struct file_set_node * node = file_set_find(&files_list, fd);
    if ( !node ) {
        error_set(ERR_CHANNEL);
        return STATUS_ERROR;
    }

    return node->ptr;
}

/* Increments the file pointer for an open file */
void
open_file_increment_ptr(const int fd) {
    struct file_set_node * node = file_set_find(&files_list, fd);
    if ( !node ) {
        ABORT("failed to find open file");
    }

    node->ptr++;
}

/* Removes a file from the open files list */
void
open_file_remove(const int fd) {
    file_set_remove(&files_list, fd);
}

/* Sets the file pointer for an open file */
int
open_file_set_ptr(const int fd, const int ptr) {
    struct file_set_node * node = file_set_find(&files_list, fd);
    if ( !node ) {
        error_set(ERR_CHANNEL);
        return STATUS_ERROR;
    }

    node->ptr = ptr;

    return STATUS_OK;
}


/*********************************************************************
 *                                                                   *
 * Colour function                                                   *
 *                                                                   *
 *********************************************************************/

#ifdef ENABLE_ANSI_COLOURS
/* Sets the "colours used" flag */
void
set_colour_used(void) {
    colours_used = true;
}
#endif


/*********************************************************************
 *                                                                   *
 * Error functions                                                   *
 *                                                                   *
 *********************************************************************/

/* Returns true if the status indicates an error */
static bool
status_error(const int status) {
    return (status >= 0) || (status == STATUS_ERROR);
}

/* Clears the current error code. */
void
error_clear(void) {
    error_register.code = ERR_NO_ERROR;
    error_register.line = 0;
}

/* Returns the current error code */
int
error_code(void) {
    return error_register.code;
}

/* Returns true if the error code is set */
bool
error_is_set(void) {
    return error_register.code != ERR_NO_ERROR;
}

/* Returns the last reported error code */
int
error_last_code(void) {
    return error_register.last_code;
}

/* Returns the last reported error line */
int
error_last_line(void) {
    return error_register.last_line;
}

/* Returns the current error line */
int
error_line(void) {
    return error_register.line;
}

/* Sets the current error code */
enum basic_error
error_set(const enum basic_error code) {
    error_register.code = code;
    error_register.line = current_line;
    error_register.last_code = code;
    error_register.last_line = current_line;
    return code;
}

/* Returns a string representation of the last reported error, or
 * NULL if there is no previous error, or if the previous error
 * code is not recognized
 */
const char *
error_string(const enum basic_error code) {
    /* Output base error message */
    switch ( code ) {
        case ERR_ACCURACY_LOST:
            return "Accuracy lost";

        case ERR_ARGUMENTS:
            return "Arguments";

        case ERR_ARRAY:
            return "Array";

        case ERR_BAD_CALL:
            return "Bad call";

        case ERR_BAD_DIM:
            return "Bad DIM";

        case ERR_BAD_HEX:
            return "Bad HEX";

        case ERR_BAD_KEY:
            return "Bad key";

        case ERR_BAD_MODE:
            return "Bad MODE";

        case ERR_BAD_PROGRAM:
            return "Bad program";

        case ERR_BAD_STRING:
            return "Bad string";

        case ERR_BLOCK:
            return "Block?";

        case ERR_BYTE:
            return "Byte";

        case ERR_CANT_MATCH_FOR:
            return "Can't match FOR";

        case ERR_CHANNEL:
            return "Channel";

        case ERR_DATA:
            return "Data?";

        case ERR_DIM_SPACE:
            return "Dim space";

        case ERR_DIVIDE_ZERO:
            return "Division by zero";

        case ERR_STRING_RANGE:
            return "$ range";

        case ERR_EOF:
            return "Eof";

        case ERR_ESCAPE:
            return "Escape";

        case ERR_EXP_RANGE:
            return "Exp range";

        case ERR_FILE:
            return "File?";

        case ERR_FOR_VARIABLE:
            return "FOR variable";

        case ERR_HEADER:
            return "Header?";

        case ERR_INDEX:
            return "Index";

        case ERR_KEY_IN_USE:
            return "Key in use";

        case ERR_LOG_RANGE:
            return "Log range";

        case ERR_MISSING_COMMA:
            return "Missing ,";

        case ERR_MISSING_QUOTE:
            return "Missing \"";

        case ERR_MISSING_RPAREN:
            return "Missing )";

        case ERR_MISSING_HASH:
            return "Missing #";

        case ERR_MISTAKE:
            return "Mistake";

        case ERR_NEGATIVE_ROOT:
            return "-ve root";

        case ERR_NO_FN:
            return "No FN";

        case ERR_NO_FOR:
            return "No FOR";

        case ERR_NO_GOSUB:
            return "No GOSUB";

        case ERR_NO_PROC:
            return "No PROC";

        case ERR_NO_REPEAT:
            return "No REPEAT";

        case ERR_NO_SUCH_FN_PROC:
            return "No such FN/PROC";

        case ERR_NO_SUCH_LINE:
            return "No such line";

        case ERR_NO_SUCH_VARIABLE:
            return "No such variable";

        case ERR_NO_TO:
            return "No TO";

        case ERR_NOT_LOCAL:
            return "Not LOCAL";

        case ERR_ON_RANGE:
            return "ON range";

        case ERR_ON_SYNTAX:
            return "ON syntax";

        case ERR_OUT_OF_DATA:
            return "Out of DATA";

        case ERR_OUT_OF_RANGE:
            return "Out of range";

        case ERR_SYNTAX:
            return "Syntax";

        case ERR_STRING_TOO_LONG:
            return "String too long";

        case ERR_SUBSCRIPT:
            return "Subscript";

        case ERR_SYNTAX_ERROR:
            return "Syntax error";

        case ERR_TOO_BIG:
            return "Too big";

        case ERR_TOO_MANY_FORS:
            return "Too many FORs";

        case ERR_TOO_MANY_GOSUBS:
            return "Too many GOSUBs";

        case ERR_TOO_MANY_REPEATS:
            return "Too many REPEATs";

        case ERR_TYPE_MISMATCH:
            return "Type mismatch";

        default:
            break;
    }

    return NULL;
}

/* Outputs an error message, including the current line number if
 * one is set
 */
void
error_output(void) {
    const char * msg = error_string(error_register.code);
    if ( !msg ) {
        if ( error_register.code == ERR_NO_ERROR ) {
            ABORTF("no error reported in %s", __func__);
        }

        ABORTF("unrecognized error code: %d", error_register.code);
    }

    fprintf(stderr, "%s", msg);

    /* Output line number if one is set */
    if ( error_register.line == 0 ) {
        fputc('\n', stderr);
    } else {
        fprintf(stderr, " at line %d\n", error_register.line);
    }
}

/* Clears the error handling statement */
void
error_stmt_clear(void) {
    if ( error_stmt != NULL ) {
        /* Although we hold a pointer to it, the error handling
         * statement still belongs to the ON ERROR statement to
         * which it is attached, so we reset it to its original
         * state but otherwise leave the responsibility of
         * freeing it to its parent statement.
         */
        error_stmt->next = NULL;
        error_stmt = NULL;
    }
}

/* Sets the error handling statement */
void
error_stmt_set(struct statement * s, struct statement * next) {
    error_stmt_clear();
    error_stmt = s;
    error_stmt->next = next;
}
