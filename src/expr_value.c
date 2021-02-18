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

/* Functions and types for expressions representing concrete values */

#include <stdint.h>
#include <stdbool.h>

#include "expr_internal.h"
#include "value.h"
#include "symbols.h"
#include "util.h"

/* Static function declarations */
static struct value * expr_eval_array(struct expr * e);
static struct value * expr_eval_constant(struct expr * e);
static struct value * expr_eval_variable(struct expr * e);


/*********************************************************************
 *                                                                   *
 * Constructor functions                                             *
 *                                                                   *
 *********************************************************************/

/* Creates a new array variable value */
struct expr *
expr_array_new(const char * id, struct expr * indices) {
    struct expr * e = expr_new(EXPR_ARRAY);
    e->subs[0] = expr_string_new(id);
    e->subs[1] = indices;
    e->eval = expr_eval_array;
    return e;
}

/* Creates a new constant value */
struct expr *
expr_constant_new(struct value *v) {
    struct expr * e = expr_new(EXPR_CONSTANT);
    e->val = value_copy(v);
    e->eval = expr_eval_constant;
    return e;
}

/* Creates a new float value */
struct expr *
expr_float_new(const double d) {
    struct expr * e = expr_new(EXPR_CONSTANT);
    e->val = value_float_new(d);
    e->eval = expr_eval_constant;
    return e;
}

/* Creates a new integer value */
struct expr *
expr_int_new(const int32_t n) {
    struct expr * e = expr_new(EXPR_CONSTANT);
    e->val = value_int_new(n);
    e->eval = expr_eval_constant;
    return e;
}

/* Creates a new string value */
struct expr *
expr_string_new(const char * s) {
    struct expr * e = expr_new(EXPR_CONSTANT);
    e->val = value_string_new(s);
    e->eval = expr_eval_constant;
    return e;
}

/* Creates a new (non-array) variable value */
struct expr *
expr_variable_new(const char * id) {
    struct expr * e = expr_new(EXPR_VARIABLE);
    e->subs[0] = expr_string_new(id);
    e->eval = expr_eval_variable;
    return e;
}


/*********************************************************************
 *                                                                   *
 * Value checking/retrieval functions                                *
 *                                                                   *
 *********************************************************************/

/* Returns the indices of an array identifier without making a
 * copy. The caller should not take ownership of the pointer,
 * or pass it to any function which does.
 */
struct expr *
expr_indices_peek(struct expr * e) {
    if ( e->type != EXPR_ARRAY ) {
        ABORTF("expression with type %d is not an array", e->type);
    }
    return e->subs[1];
}

/* Returns true is an expression is an array */
bool
expr_is_array(struct expr * e) {
    return e->type == EXPR_ARRAY;
}

/* Returns true if an expression is a constant */
bool
expr_is_constant(struct expr * e) {
    return e->type == EXPR_CONSTANT;
}

/* Returns true is an expression is a (non-array) variable */
bool
expr_is_variable(struct expr * e) {
    return e->type == EXPR_VARIABLE;
}

/* Returns the name of a variable identifier without making a
 * copy. The caller should not take ownership of the pointer,
 * or pass it to any function which does.
 */
char *
expr_id_peek(struct expr * e) {
    if ( e->type != EXPR_VARIABLE && e->type != EXPR_ARRAY ) {
        ABORTF("expression with type %d is not a variable", e->type);
    }
    return value_string_peek(e->subs[0]->val);
}


/*********************************************************************
 *                                                                   *
 * Static evaluation functions                                       *
 *                                                                   *
 *********************************************************************/

/* Evaluates an array expression */
static struct value *
expr_eval_array(struct expr * e) {
    struct value * vals = NULL;
    struct expr * dims = e->subs[1];
    while ( dims ) {
        struct value * dim = expr_eval(dims);
        if ( !dim ) {
            value_free(vals);
            return NULL;
        }
        vals = value_append(vals, dim);
        dims = expr_next(dims);
    }

    struct value * v = symbol_array_eval(expr_id_peek(e), vals);
    value_free(vals);
    return v;
}

/* Evaluates a constant expression */
static struct value *
expr_eval_constant(struct expr * e) {
    return value_copy(e->val);
}

/* Evaluates a (non-array) variable expression */
static struct value *
expr_eval_variable(struct expr * e) {
    return symbol_variable_eval(expr_id_peek(e));
}
