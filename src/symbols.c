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

/* A symbol table */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

#include "symbols.h"
#include "statements.h"
#include "value.h"
#include "runtime.h"
#include "util.h"
#include "hash.h"

#define VAR_NAME_COUNT "COUNT"
#define VAR_NAME_TIME "TIME"
#define NUM_BUCKETS (257)

/* Symbol types */
enum symbol_type {
    SYMBOL_UNINITIALIZED = 1,
    SYMBOL_ARRAY,
    SYMBOL_FLOAT,
    SYMBOL_INTEGER,
    SYMBOL_PROCEDURE,
    SYMBOL_STRING,
};

/* Array type */
struct array {
    size_t size;
    enum symbol_type type;
    struct value * dims;
    struct symbol ** data;
};

/* Tagged union entry for a single symbol */
struct symbol {
    char * id;
    enum symbol_type type;
    union {
        char * s;
        double f;
        int n;
        struct statement * stmt;
        struct array * array;
    } u;
    struct symbol * next;
};

/* Symbol table. The base symbol table is statically allocated,
 * but additional symbol table frames are added when procedures
 * and functions are called, to hold their local variables. In
 * general, when retrieving a variable value, the stack frames
 * will be searched from top to bottom. When adding a new variable,
 * by default it will be added in the base table, but arguments
 * and variables declared LOCAL will be placed in the top stack
 * frame. Non-variable symbols such as functions and procedures
 * are always stored in the base table.
 */
static struct symtable {
    struct symbol * buckets[NUM_BUCKETS];
    struct symtable * prev;
} table, * top_frame = &table;

/* Static function declarations */
static void clear_buckets(struct symtable * t);
static void free_buckets(struct symtable * t);
static bool have_frames(void);
static struct symbol * symbol_copy(struct symbol * id);
static struct symbol * symbol_find(const char * id);
static struct symbol * symbol_find_frame(const char * id, struct symtable * t);
static void symbol_free(struct symbol * s);
static void symbol_insert(struct symbol * s);
static void symbol_insert_frame(struct symbol * s, struct symtable * t);
static void symbol_move(struct symbol * dst, struct symbol * src);

static int procedure_add(const char * id, struct statement * stmt);

static int resident_assign(const int c, struct value * v);
static struct value * resident_eval(const int c);
static int resident_index(const int c);

static int array_index(struct value * dims, struct value * indices);
static size_t array_size(struct value * v);

static int variable_add(const char * id, struct value * v);
static int variable_add_frame(const char * id,
        struct value * v, struct symtable * t);
static struct value * variable_get(const char * id);
static void variable_set(struct symbol * s, struct value * v);

/* Storage for resident integer variables @% and A%-Z%.
 * @% has an initial value of 0x90a, and the others have
 * initial values of zero.
 */
static int32_t residents[27] = { 0x90a };

/* Datum holds a clock value that is used to calculate a difference
 * for the TIME pseudo-variable. set_time_value can be set by assigning
 * a value to the TIME pseudo-variable. The effect is that reading from
 * the TIME pseudo-variable will return the number of 1/100ths of
 * seconds since TIME was last set.
 */
static struct timespec datum;
static long set_time_value;


/*********************************************************************
 *                                                                   *
 * Public symbol table functions                                     *
 *                                                                   *
 *********************************************************************/


/* Passes arguments onto the stack. params should be a list of
 * expressions of variable type, and args should be an equally-
 * sized list of expressions. A new stack frame will be created,
 * and each item of args will be placed onto the stack in a local
 * variable with the same name as the item of the same index
 * in params.
 */
int
pass_arguments(struct expr * params, struct expr * args) {
    /* Assign procedure arguments to local variables */
    struct expr * param = params;
    struct expr * arg = args;
    struct value * vals = NULL;

    /* Although all arguments will have been previously
     * evaluated, recursive function calls may have evaluated
     * them again and left them in a state relevant a stack
     * frame other than this one. Therefore, we must evaluate
     * them again.
     */
    do {
        if ( (param == NULL) != (arg == NULL) ) {
            /* Mismatched number of parameters and arguments */
            error_set(ERR_ARGUMENTS);
            value_free(vals);
            return ERR_ARGUMENTS;
        } else if ( !param ) {
            /* End of argument list - first iteration only */
            break;
        }

        struct value * v = expr_eval(arg);
        if ( !v ) {
            value_free(vals);
            return STATUS_ERROR;
        }

        vals = value_append(vals, v);

        /* Advance to next argument */
        param = expr_next(param);
        arg = expr_next(arg);
    } while ( param || arg );

    /* Only AFTER they've all been evaluated, push a new stack
     * frame and add them as local variables.
     */
    symbol_table_push_frame();

    param = params;
    arg = args;
    struct value * val = vals;
    while ( val ) {
        int status = symbol_variable_assign_local(expr_id_peek(param), val);
        if ( status != STATUS_OK ) {
            symbol_table_pop_frame();
            value_free(vals);
            return status;
        }

        /* Advance to next argument */
        param = expr_next(param);
        arg = expr_next(arg);
        val = value_next(val);
    }

    value_free(vals);

    return ERR_NO_ERROR;
}

/* Returns a procedure address, or NULL if it's not found */
struct statement *
symbol_proc_call(const char * id) {
    struct symbol * s = symbol_find(id);
    if ( !s || s->type != SYMBOL_PROCEDURE ) {
        return NULL;
    }

    return s->u.stmt;
}

/* Adds a procedure to the symbol table */
int
symbol_proc_define(const char * id, struct statement * stmt){
    return procedure_add(id, stmt);
}

/* Clears all variables from the symbol table (but leaves
 * any other symbols intact). Resident integer variables
 * are not affected, and the number of stack frames remains
 * the same.
 */
void
symbol_table_clear(void) {
    struct symtable * current = top_frame;
    while ( current ) {
        clear_buckets(current);
        current = current->prev;
    }
}

/* Frees the resources associated with the symbol table,
 * popping all additional stack frames.
 */
void
symbol_table_free(void) {
    while ( have_frames() ) {
        symbol_table_pop_frame();
    }
    free_buckets(&table);
}

/* Performs symbol table initialization */
void
symbol_table_init(void) {
    errno = 0;
    if ( clock_gettime(CLOCK_REALTIME, &datum) == -1 ) {
        ABORTF("clock_gettime failed: %s\n", strerror(errno));
    }
}

/* Pops a frame from the top of the symbol table stack */
void
symbol_table_pop_frame(void) {
    if ( !have_frames() ) {
        ABORT("symbol table stack frame underflow");
    }

    struct symtable * tmp = top_frame->prev;
    free_buckets(top_frame);
    free(top_frame);
    top_frame = tmp;
}

/* Pushes a new frame onto the symbol table stack */
void
symbol_table_push_frame(void) {
    struct symtable * frame = x_calloc(1, sizeof *frame);
    frame->prev = top_frame;
    top_frame = frame;
}

/* Assigns a value to an array element */
int
symbol_array_assign(const char * id, struct value * indices, struct value * v) {
    struct symbol * s = symbol_find(id);
    if ( !s ) {
        error_set(ERR_ARRAY);
        return STATUS_ERROR;
    }

    if ( s->type != SYMBOL_ARRAY ) {
        error_set(ERR_ARRAY);
        return STATUS_ERROR;
    }

    const int index = array_index(s->u.array->dims, indices);
    if ( index == STATUS_ERROR ) {
        return STATUS_ERROR;
    }

    struct symbol * e = s->u.array->data[index];
    switch ( e->type ) {
        case SYMBOL_FLOAT:
            if ( !value_is_int(v) && !value_is_float(v) ) {
                error_set(ERR_TYPE_MISMATCH);
                return STATUS_ERROR;
            }
            e->u.f = value_float(v);
            break;

        case SYMBOL_INTEGER:
            if ( !value_is_int(v) ) {
                error_set(ERR_TYPE_MISMATCH);
                return STATUS_ERROR;
            }
            e->u.n = value_int(v);
            break;

        case SYMBOL_STRING:
            if ( !value_is_string(v) ) {
                error_set(ERR_TYPE_MISMATCH);
                return STATUS_ERROR;
            }
            free(e->u.s);
            e->u.s = value_string(v);
            break;

        default:
            ABORTF("unexpected symbol type: %d\n", e->type);
    }

    return STATUS_OK;
}

/* Dimensions an array */
int
symbol_array_dimension(const char * id, struct value * v) {
    if ( symbol_find(id) ) {
        error_set(ERR_BAD_DIM);
        value_free(v);
        return STATUS_ERROR;
    }

    /* Determine type of array from variable name */
    enum symbol_type type;
    if ( variable_name_is_real(id) ) {
        type = SYMBOL_FLOAT;
    } else if ( variable_name_is_resident(id) || variable_name_is_integer(id) ) {
        type = SYMBOL_INTEGER;
    } else if ( variable_name_is_string(id) ) {
        type = SYMBOL_STRING;
    } else {
        ABORT("unexpected symbol type");
    }

    /* Allocate array of appropriate size */
    const size_t size = array_size(v);

    struct array * array = x_malloc(sizeof *array);
    array->size = size;
    array->dims = v;
    array->data = x_malloc(sizeof *array->data * size);
    array->type = type;

    /* Populate array elements with appropriately
     * typed zero values
     */
    for ( size_t i = 0; i < size; i++ ) {
        struct symbol * s = x_malloc(sizeof *s);
        s->id = NULL;
        s->next = NULL;
        s->type = type;

        switch ( type ) {
            case SYMBOL_FLOAT:
                s->u.f = 0.0;
                break;

            case SYMBOL_INTEGER:
                s->u.n = 0;
                break;

            case SYMBOL_STRING:
                s->u.s = x_strdup("");
                break;

            default:
                ABORTF("unexpected symbol type: %d", type);
        }

        array->data[i] = s;
    }

    /* Create temporary symbol and insert into table */
    struct symbol s = (struct symbol){
        .type = SYMBOL_ARRAY,
        .u.array = array,
        .id = (char *) id, /* symbol_insert_frame will strdup this */
        .next = NULL,
    };

    symbol_insert_frame(&s, &table);

    return STATUS_OK;
}

/* Evaluates an array reference */
struct value *
symbol_array_eval(const char * id, struct value * indices) {
    struct symbol * s = symbol_find(id);
    if ( !s ) {
        error_set(ERR_ARRAY);
        return NULL;
    }

    if ( s->type != SYMBOL_ARRAY ) {
        error_set(ERR_ARRAY);
        return NULL;
    }

    const int index = array_index(s->u.array->dims, indices);
    if ( index == STATUS_ERROR ) {
        return NULL;
    }

    struct symbol * e = s->u.array->data[index];

    struct value * result;
    switch ( e->type ) {
        case SYMBOL_FLOAT:
            result = value_float_new(e->u.f);
            break;

        case SYMBOL_INTEGER:
            result = value_int_new(e->u.n);
            break;

        case SYMBOL_STRING:
            result = value_string_new(e->u.s);
            break;

        default:
            ABORTF("unexpected symbol type: %d", e->type);
    }

    return result;
}

/* Assigns the value of an expression to a global variable */
int
symbol_variable_assign(const char * id, struct value * v) {
    if ( variable_name_is_resident(id) ) {
        /* Resident integer variables are not stored
         * separately from the symbol table */
        return resident_assign(id[0], v);
    } else if ( !strcmp(id, VAR_NAME_TIME) ) {
        /* TIME pseudo-variable */
        if ( !value_is_numeric(v) ) {
            error_set(ERR_TYPE_MISMATCH);
            return ERR_TYPE_MISMATCH;
        }

        /* Store the value and reset the clock datum */
        set_time_value = value_int(v);
        if ( clock_gettime(CLOCK_REALTIME, &datum) == -1 ) {
            ABORTF("clock_gettime failed: %s\n", strerror(errno));
        }

        return STATUS_OK;
    }

    return variable_add(id, v);
}

/* Assigns the value of an expression to a local variable. If
 * the expression is NULL, a zero value of a type appropriate
 * to the variable name is assigned.
 */
int
symbol_variable_assign_local(const char * id, struct value * v) {
    if ( variable_name_is_resident(id) ) {
        /* Resident integer variables are non-local by nature,
         * so treat them the same as always
         */
        return resident_assign(id[0], v);
    }

    if ( v ) {
        return variable_add_frame(id, v, top_frame);
    }

    if ( variable_name_is_string(id) ) {
        v = value_string_new("");
    } else {
        v = value_int_new(0);
    }

    int status = variable_add_frame(id, v, top_frame);
    value_free(v);

    return status;
}

/* Evaluates a variable stored in the symbol table */
struct value *
symbol_variable_eval(const char * id) {
    if ( variable_name_is_resident(id) ) {
        /* Resident integer variables are not stored in the symbol table */
        return resident_eval(id[0]);
    } else if ( !strcmp(id, VAR_NAME_TIME) ) {
        /* TIME pseudo-variable */
        errno = 0;

        /* Get the current time and compare it with the time datum,
         * and add the difference in 1/100ths of seconds to the
         * last stored value of TIME.
         */
        struct timespec tv;
        if ( clock_gettime(CLOCK_REALTIME, &tv) == -1 ) {
            ABORTF("clock_gettime failed: %s\n", strerror(errno));
        }
        long hsecs = (tv.tv_sec * 100.0 + tv.tv_nsec / 10000000.0)
            - (datum.tv_sec * 100.0 + datum.tv_nsec / 10000000.0);

        return value_int_new(set_time_value + hsecs);
    } else if ( !strcmp(id, VAR_NAME_COUNT) ) {
        return value_int_new(print_count());
    }

    return variable_get(id);
}


/*********************************************************************
 *                                                                   *
 * Variable name functions                                           *
 *                                                                   *
 *********************************************************************/

/* Returns true if a variable name refers to a non-resident
 * integer variable
 */
bool
variable_name_is_integer(const char * s) {
    const size_t l = strlen(s);
    if ( l < 2 || s[l-1] != '%' ) {
        return false;
    }

    return l > 2 || islower(s[1]);
}

/* Returns true if a variable name refers to a real variable */
bool
variable_name_is_real(const char * s) {
    const size_t l = strlen(s);
    return l > 0 && s[l-1] != '$' && s[l-1] != '%';
}

/* Returns true if a variable name refers to a resident
 * integer variable
 */
bool
variable_name_is_resident(const char * s) {
    const size_t l = strlen(s);
    return l == 2 && (isupper(s[0]) || s[0] == '@') && s[1] == '%';
}

/* Returns true if a variable name refers to a string variable */
bool
variable_name_is_string(const char * s) {
    const size_t l = strlen(s);
    return l > 0 && s[l-1] == '$';
}


/*********************************************************************
 *                                                                   *
 * Public number format functions                                    *
 *                                                                   *
 *********************************************************************/

/* Returns the current number format from the @% variable */
enum number_format
format_number(void) {
    switch ( (residents[resident_index('@')] & 0xFF0000) >> 16 ) {
        case 1:
            return FORMAT_SCIENTIFIC;

        case 2:
            return FORMAT_FIXED;
    }

    /* Normal is the default */
    return FORMAT_NORMAL;
}

/* Returns the current number of significant figures/decimal
 * places from the @% variable. The maximum number of significant
 * figures is 10.
 */
int
format_places(void) {
    int p = ((residents[resident_index('@')] & 0xFF00) >> 8);
    return (p < 1 || p > 10) ? 10 : p;
}

/* Resets the @% variable to its default value */
void
format_reset(void) {
    residents[resident_index('@')] = 0x90a;
}

/* Returns the current field width from the @% variable */
int
format_width(void) {
    return residents[resident_index('@')] & 0xFF;
}


/*********************************************************************
 *                                                                   *
 * Static general symbol table functions                             *
 *                                                                   *
 *********************************************************************/

/* Clears all variables from a symbol table frame (but leaves
 * any other symbols intact). Resident integer variables are
 * not affected. The frame is not popped, so this function can
 * be called on the statically allocated base symbol table.
 */
static void
clear_buckets(struct symtable * t) {
    for ( int i = 0; i < NUM_BUCKETS; i++ ) {
        if ( t->buckets[i] != NULL ) {
            struct symbol * current = t->buckets[i];
            struct symbol * previous = NULL;
            while ( current ) {
                struct symbol * tmp = current->next;
                switch ( current->type ) {
                    case SYMBOL_FLOAT:
                    case SYMBOL_INTEGER:
                    case SYMBOL_STRING:
                        if ( !previous ) {
                            t->buckets[i] = tmp;
                        } else {
                            previous->next = tmp;
                        }
                        symbol_free(current);
                        break;

                    default:
                        /* Do nothing with non-variables */
                        previous = current;
                        break;
                }
                current = tmp;
            }
        }
    }
}

/* Frees the buckets in a specific symbol table frame. The
 * frame itself is not affected, so this function can be
 * called on the statically allocated base symbol table.
 */
static void
free_buckets(struct symtable * t) {
    for ( int i = 0; i < NUM_BUCKETS; i++ ) {
        if ( t->buckets[i] != NULL ) {
            struct symbol * current = t->buckets[i];
            while ( current ) {
                struct symbol * tmp = current->next;
                symbol_free(current);
                current = tmp;
            }
            t->buckets[i] = NULL; /* So we could reuse */
        }
    }
}

/* Returns true if there are currently stack frames on top
 * of the base symbol table.
 */
static bool
have_frames(void) {
    return top_frame != &table;
}

/* Copies a symbol into a newly-allocated one */
static struct symbol *
symbol_copy(struct symbol * s) {
    struct symbol * new_sym = x_malloc(sizeof *new_sym);
    new_sym->id = x_strdup(s->id);
    new_sym->type = s->type;
    new_sym->u = s->u;
    new_sym->next = NULL;
    return new_sym;
}

/* Returns a symbol, or NULL if it cannot be found. Stack frames
 * are checked sequentially, from the top down.
 */
static struct symbol *
symbol_find(const char * id) {
    struct symtable * frame = top_frame;
    while ( frame ) {
        struct symbol * found = symbol_find_frame(id, frame);
        if ( found ) {
            return found;
        }
        frame = frame->prev;
    }

    return NULL;
}

/* Returns a symbol from a specific frame, or NULL if it cannot
 * be found
 */
static struct symbol *
symbol_find_frame(const char * id, struct symtable * t) {
    struct symbol * current = t->buckets[djb2hash(id, NUM_BUCKETS)];
    while ( current ) {
        if ( !strcmp(current->id, id) ) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

/* Frees the resources associated with a single symbol */
static void
symbol_free(struct symbol * s) {
    size_t size;

    free(s->id);

    switch ( s->type ) {
        case SYMBOL_ARRAY:
            size = array_size(s->u.array->dims);
            for ( size_t i = 0; i < size; i++ ) {
                symbol_free(s->u.array->data[i]);
            }
            value_free(s->u.array->dims);
            free(s->u.array->data);
            free(s->u.array);
            break;

        case SYMBOL_STRING:
            free(s->u.s);
            break;

        default:
            /* No specific processing */
            break;
    }

    free(s);
}

/* Inserts a global symbol into the symbol table, or replaces an
 * existing global symbol.
 */
static void
symbol_insert(struct symbol * s) {
    symbol_insert_frame(s, &table);
}

/* Inserts a symbol into a specific stack frame, or replaces an
 * existing symbol in that frame.
 */
static void
symbol_insert_frame(struct symbol * s, struct symtable * t) {
    const size_t hash = djb2hash(s->id, NUM_BUCKETS);
    struct symbol * current = t->buckets[hash];
    if ( current == NULL ) {
        /* Bucket does not exist, so create it */
        t->buckets[hash] = symbol_copy(s);
    } else {
        struct symbol * previous = NULL;

        while ( true ) {
            if ( !strcmp(current->id, s->id) ) {
                /* Symbol exists, so replace it */
                symbol_move(current, s);

                break;
            }

            if ( !current->next ) {
                /* Symbol doesn't exist, so append it */
                current->next = symbol_copy(s);

                break;
            }

            /* Keep looking */
            previous = current;
            current = current->next;
        }
    }
}

/* Moves the values of one symbol into another, without allocating.
 * The destination symbol should have the same ID as the source
 * symbol, as it will not be updated.
 */
static void
symbol_move(struct symbol * dst, struct symbol * src) {
    if ( dst->type == SYMBOL_STRING ) {
        free(dst->u.s);
    }
    dst->type = src->type;
    dst->u = src->u;
}


/*********************************************************************
 *                                                                   *
 * Static procedure functions                                        *
 *                                                                   *
 *********************************************************************/

/* Adds a procedure to the symbol table, or replaces an existing one */
static int
procedure_add(const char * id, struct statement * stmt) {
    struct symbol s;
    s.type = SYMBOL_PROCEDURE;
    s.u.stmt = stmt;
    s.id = (char *)id;
    s.next = NULL;
    symbol_insert(&s);

    return STATUS_OK;
}


/*********************************************************************
 *                                                                   *
 * Static resident integer functions                                 *
 *                                                                   *
 *********************************************************************/

/* Assigns a value to a resident integer variable */
static int
resident_assign(const int c, struct value * v) {
    if ( !value_is_int(v) ) {
        error_set(ERR_TYPE_MISMATCH);
        return STATUS_ERROR;
    }

    residents[resident_index(c)] = value_int(v);

    return STATUS_OK;
}

/* Evaluates a resident integer variable */
static struct value *
resident_eval(const int c) {
    return value_int_new(residents[resident_index(c)]);
}

/* Returns the storage index for a resident integer variable name. The primary
 * advantage of resident integer variables is their speed, so they are
 * implemented with a large and unwieldy-looking lookup table.
 */
static int
resident_index(const int c) {
    int n;

    switch ( c ) {
        case '@':
            n = 0;
            break;

        case 'A':
            n = 1;
            break;

        case 'B':
            n = 2;
            break;

        case 'C':
            n = 3;
            break;

        case 'D':
            n = 4;
            break;

        case 'E':
            n = 5;
            break;

        case 'F':
            n = 6;
            break;

        case 'G':
            n = 7;
            break;

        case 'H':
            n = 8;
            break;

        case 'I':
            n = 9;
            break;

        case 'J':
            n = 10;
            break;

        case 'K':
            n = 11;
            break;

        case 'L':
            n = 12;
            break;

        case 'M':
            n = 13;
            break;

        case 'N':
            n = 14;
            break;

        case 'O':
            n = 15;
            break;

        case 'P':
            n = 16;
            break;

        case 'Q':
            n = 17;
            break;

        case 'R':
            n = 18;
            break;

        case 'S':
            n = 19;
            break;

        case 'T':
            n = 20;
            break;

        case 'U':
            n = 21;
            break;

        case 'V':
            n = 22;
            break;

        case 'W':
            n = 23;
            break;

        case 'X':
            n = 24;
            break;

        case 'Y':
            n = 25;
            break;

        case 'Z':
            n = 26;
            break;

        default:
            ABORTF("unexpected resident variable name: %c (%d)", c, c);
    }

    return n;
}


/*********************************************************************
 *                                                                   *
 * Static array functions                                            *
 *                                                                   *
 *********************************************************************/

/* Calculates an array data index, validating a set of
 * indices in the process
 */
static int
array_index(struct value * dims, struct value * indices) {
    size_t size = 1;

    /* Verify type, number and value of indices */
    struct value * d = dims;
    struct value * i = indices;
    while ( d && i ) {
        if ( value_int(i) < 0 || value_int(i) > value_int(d) ) {
            error_set(ERR_SUBSCRIPT);
            return STATUS_ERROR;
        }

        /* Store size for index calculation */
        size *= value_int(d)+1;

        d = value_next(d);
        i = value_next(i);

        if ( !d && i ) {
            error_set(ERR_MISSING_RPAREN);
            return STATUS_ERROR;
        } else if ( d && !i ) {
            error_set(ERR_ARRAY);
            return STATUS_ERROR;
        }
    }

    /* Calculate data array index */
    int index = 0;
    
    d = dims;
    i = indices;
    while ( d && i ) {
        size /= (value_int(d) + 1);
        index += value_int(i) * size;

        d = value_next(d);
        i = value_next(i);
    }

    return index;
}

/* Calculates the size of an array from its dimensions.
 * Note that in BBC BASIC II arrays are zero-indexed, but
 * n is a valid index for DIM var(n), so we actually need
 * to allocate n+1 elements, and similarly for any higher
 * dimensions.
 */

static size_t
array_size(struct value * dims) {
    size_t size = 1;

    while ( dims ) {
        size *= value_int(dims) + 1;
        dims = value_next(dims);
    }

    return size;
}


/*********************************************************************
 *                                                                   *
 * Static variable functions                                         *
 *                                                                   *
 *********************************************************************/

/* Adds a variable to the symbol table. The stack frames are
 * checked from top to bottom, and if the symbol is already
 * defined, the value in the highest stack frame in which it
 * is found will be replaced. The effect is assigning to the
 * existing variable of closest enclosing scope. If the symbol
 * is not already defined, it is added as a global variable.
 */
static int
variable_add(const char * id, struct value * v) {
    if ( !have_frames() ) {
        return variable_add_frame(id, v, &table);
    }

    struct symtable * frame = top_frame;
    while ( frame->prev ) {
        if ( symbol_find_frame(id, frame) ) {
            return variable_add_frame(id, v, frame);
        }
        frame = frame->prev;
    }

    return variable_add_frame(id, v, &table);
}

/* Adds a variable to a specific stack frame, or replaces an
 * existing one
 */
static int
variable_add_frame(const char * id, struct value * v, struct symtable * t) {
    /* Perform type-checking based on ID suffix */
    if ( variable_name_is_string(id) ) {
        if ( !value_is_string(v) ) {
            error_set(ERR_TYPE_MISMATCH);
            return STATUS_ERROR;
        }
    } else if ( variable_name_is_integer(id) ) {
        if ( !value_is_int(v) ) {
            error_set(ERR_TYPE_MISMATCH);
            return STATUS_ERROR;
        }
    } else {
        if ( !value_is_numeric(v) ) {
            error_set(ERR_TYPE_MISMATCH);
            return STATUS_ERROR;
        }
    }

    struct symbol s;
    s.id = (char *)id;
    s.next = NULL;
    variable_set(&s, v);
    symbol_insert_frame(&s, t);

    return STATUS_OK;
}

/* Returns the value of a variable in the closest enclosing scope */
static struct value *
variable_get(const char * id) {
    struct symbol * current = symbol_find(id);
    if ( !current ) {
        error_set(ERR_NO_SUCH_VARIABLE);
        return NULL;
    }

    struct value * result;
    switch ( current->type ) {
        case SYMBOL_FLOAT:
            result = value_float_new(current->u.f);
            break;

        case SYMBOL_INTEGER:
            result = value_int_new(current->u.n);
            break;

        case SYMBOL_STRING:
            result = value_string_new(current->u.s);
            break;

        case SYMBOL_PROCEDURE:
            /* Only return variables through this function */
            error_set(ERR_NO_SUCH_VARIABLE);
            return NULL;

        default:
            ABORTF("unexpected symbol type: %d", current->type);
    }

    return result;
}

/* Sets the type and value of a variable */
static void
variable_set(struct symbol * s, struct value * v) {
    if ( value_is_float(v) ) {
        s->type = SYMBOL_FLOAT;
        s->u.f = value_float(v);
    } else if ( value_is_int(v) ) {
        s->type = SYMBOL_INTEGER;
        s->u.n = value_int(v);
    } else if ( value_is_string(v) ) {
        s->type = SYMBOL_STRING;
        s->u.s = value_string(v);
    } else {
        ABORT("unexpected expression type");
    }
}
