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

/* Functions and types for creating and evaluating expressions,
 * including values, operators and functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "expr_internal.h"
#include "value.h"
#include "runtime.h"
#include "util.h"

/* Appends tail to head, and returns head. If head is NULL,
 * tail is returned.
 */
struct expr *
expr_append(struct expr * head, struct expr * tail) {
    if ( head == NULL ) {
        /* Enable calling on NULL list */
        return tail;
    }

    struct expr * current = head;
    while ( current->next ) {
        current = current->next;
    }
    current->next = tail;

    return head;
}

/* Evaluates an expression */
struct value *
expr_eval(struct expr * e) {
    if ( !e->eval ) {
        ABORT("expression has no eval method");
    }

    return e->eval(e);
}

/* Returns the next expression in a list, or NULL if there
 * is no next expression.
 */
struct expr *
expr_next(struct expr * e) {
    return e->next;
}

/* Frees the resources associated with an expression */
void
expr_free(struct expr * e) {
    /* Allow expression to be NULL to make recursion easier */
    if ( !e ) {
        return;
    }

    /* Free value, if present */
    if ( e->val ) {
        value_free(e->val);
    }

    /* Free any subexpressions */
    for ( size_t i = 0; i < EXPR_NUM_SUBS; i++ ) {
        expr_free(e->subs[i]);
    }

    /* Free any subsequent expressions if this is a list */
    if ( e->next ) {
        expr_free(e->next);
    }

    free(e);
}

/* Base constructor for an empty expression */
struct expr *
expr_new(enum expr_type t) {
    struct expr * e = x_malloc(sizeof *e);
    e->type = t;
    e->val = NULL;
    e->eval = NULL;
    e->next = NULL;

    for ( size_t i = 0; i < EXPR_NUM_SUBS; i++ ) {
        e->subs[i] = NULL;
    }

    return e;
}
