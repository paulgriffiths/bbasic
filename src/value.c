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

/* Functions and types for representing typed values */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>

#include "value.h"
#include "symbols.h"
#include "util.h"

/* Value types */
enum value_type {
    VALUE_FLOAT = 1,
    VALUE_INT = 2,
    VALUE_STRING = 3
};

/* Tagged union representing a typed value */
struct value {
    enum value_type type;
    union {
        double f;
        int32_t n;
        char * s;
    } value;
    struct value * next;
};

/* Static function declarations */
static struct value * value_new(void);


/*********************************************************************
 *                                                                   *
 * Constructor functions                                             *
 *                                                                   *
 *********************************************************************/

/* Copies a value */
struct value *
value_copy(struct value * v) {
    if ( value_is_float(v) ) {
        return value_float_new(v->value.f);
    } else if ( value_is_int(v) ) {
        return value_int_new(v->value.n);
    } else if ( value_is_string(v) ) {
        return value_string_new(v->value.s);
    }

    ABORTF("unrecognized value type: %d\n", v->type);
}

/* Constructs a new floating point value */
struct value *
value_float_new(const double f) {
    struct value * v = value_new();
    *v = (struct value){ .type = VALUE_FLOAT, .value.f = f};
    return v;
}

/* Constructs a new integer value */
struct value *
value_int_new(const int32_t n) {
    struct value * v = value_new();
    *v = (struct value){ .type = VALUE_INT, .value.n = n};
    return v;
}

/* Constructs a new string value */
struct value *
value_string_new(const char * s) {
    struct value * v = value_new();
    *v = (struct value){ .type = VALUE_STRING, .value.s = x_strdup(s)};
    return v;
}


/*********************************************************************
 *                                                                   *
 * Setter function                                                   *
 *                                                                   *
 *********************************************************************/

/* Returns the float value */
double
value_float(struct value * v) {
    if ( !value_is_numeric(v) ) {
        ABORTF("value with type %d is not numeric\n", v->type);
    }
    return value_is_float(v) ? v->value.f : (double) v->value.n;
}

/* Returns the integer value */
int32_t
value_int(struct value * v) {
    if ( !value_is_numeric(v) ) {
        ABORTF("value with type %d is not numeric\n", v->type);
    }
    return value_is_int(v) ? v->value.n : (int32_t) v->value.f;
}

/* Returns the string value without making a copy. The caller
 * should not take ownership of the pointer, or pass it to any
 * function which does.
 */
char *
value_string_peek(struct value * v) {
    if ( v->type != VALUE_STRING ) {
        ABORTF("value with type %d is not a string\n", v->type);
    }
    return v->value.s;
}

/* Returns a copy of the string value */
char *
value_string(struct value * v) {
    if ( v->type != VALUE_STRING ) {
        ABORTF("value with type %d is not a string\n", v->type);
    }
    return x_strdup(v->value.s);
}


/*********************************************************************
 *                                                                   *
 * Type checking functions                                           *
 *                                                                   *
 *********************************************************************/

/* Returns true if a value has a float type */
bool
value_is_float(struct value * v) {
    return v->type == VALUE_FLOAT;
}

/* Returns true if a value has a integer type */
bool
value_is_int(struct value * v) {
    return v->type == VALUE_INT;
}

/* Returns true if a value has a numeric type */
bool
value_is_numeric(struct value * v) {
    return value_is_float(v) || value_is_int(v);
}

/* Returns true if a value has a string type */
bool
value_is_string(struct value * v) {
    return v->type == VALUE_STRING;
}

/* Returns true if a value has a numeric type and is
 * equal to zero.
 */
bool
value_is_zero(struct value * v) {
    return (value_is_int(v) && value_int(v) == 0) ||
        (value_is_float(v) && value_float(v) == 0.0);
}


/*********************************************************************
 *                                                                   *
 * List functions                                                    *
 *                                                                   *
 *********************************************************************/

/* Appends tail to head, and returns head. If head is NULL,
 * tail is returned.
 */
struct value *
value_append(struct value * head, struct value * tail) {
    if ( head == NULL ) {
        /* Enable calling on NULL list */
        return tail;
    }

    struct value * current = head;
    while ( current->next ) {
        current = current->next;
    }
    current->next = tail;

    return head;
}

/* Returns the next value in a list, or NULL if there
 * is no next value.
 */
struct value *
value_next(struct value * v) {
    return v->next;
}


/*********************************************************************
 *                                                                   *
 * Stringify functions                                               *
 *                                                                   *
 *********************************************************************/

/* Returns a string representation of a value. If format is
 * true, the value of @% is used to specify a field width
 * for numbers.
 */
char *
value_to_string(struct value * v, const bool format) {
    if ( !v ) {
        ABORT("value is NULL");
    }

    char * s;
    if ( value_is_numeric(v) ) {
        const int width = format_width();
        const int places = format_places();

        if ( format ) {
            /* With field width specifiers */

            switch ( format_number() ) {
                case FORMAT_NORMAL:
                    s = x_msprintf("%*.9G", width, places, value_float(v));
                    break;

                case FORMAT_SCIENTIFIC:
                    /* Note this probably won't match the format on
                     * the BBC Micro, where printing 2.345 with
                     * @%=&010209 would give 2.3E0, where 2.3E+00
                     * is more normal here. We'll accept the difference
                     * for now.
                     */
                    s = x_msprintf("%*.*E", width, places == 0 ? 0 : places - 1, value_float(v));
                    break;

                case FORMAT_FIXED:
                    s = x_msprintf("%*.*F", width, places, value_float(v));
                    break;

                default:
                    ABORTF("unexpected number format: %d", format_number());
            }
        } else {
            /* No field width specifiers */

            switch ( format_number() ) {
                case FORMAT_NORMAL:
                    s = x_msprintf("%.9G", places, value_float(v));
                    break;

                case FORMAT_SCIENTIFIC:
                    /* Note this probably won't match the format on
                     * the BBC Micro, where printing 2.345 with
                     * @%=&010209 would give 2.3E0, where 2.3E+00
                     * is more normal here. We'll accept the difference
                     * for now.
                     */
                    s = x_msprintf("%.*E", places == 0 ? 0 : places - 1, value_float(v));
                    break;

                case FORMAT_FIXED:
                    s = x_msprintf("%.*F", places, value_float(v));
                    break;

                default:
                    ABORTF("unexpected number format: %d", format_number());
            }
        }
    } else if ( value_is_string(v) ) {
        s = value_string(v);
    } else {
        ABORT("unexpected value type");
    }

    return s;
}


/*********************************************************************
 *                                                                   *
 * Destructor function                                               *
 *                                                                   *
 *********************************************************************/

/* Frees any resources associated with an expression value */
void
value_free(struct value * v) {
    if ( v ) {
        if ( value_is_string(v) ) {
            free(v->value.s);
        }
        if ( v->next ) {
            value_free(v->next);
        }
        free(v);
    }
}


/*********************************************************************
 *                                                                   *
 * Static sub-constructor functions                                  *
 *                                                                   *
 *********************************************************************/

static struct value *
value_new(void) {
    struct value * v = x_malloc(sizeof *v);
    v->next = NULL;
    return v;
}
