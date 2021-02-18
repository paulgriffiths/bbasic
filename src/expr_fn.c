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

#include <stdlib.h>
#include "expr_internal.h"
#include "runtime.h"
#include "statements.h"
#include "symbols.h"

/* Static function declarations */
static struct value * expr_eval_fn(struct expr * e);


/*********************************************************************
 *                                                                   *
 * Constructor function                                              *
 *                                                                   *
 *********************************************************************/

/* Creates a new FN expression value */
struct expr *
expr_fn_new(const char * id, struct expr * args) {
    struct expr * e = expr_new(EXPR_FN);
    e->subs[0] = args;
    e->subs[1] = expr_string_new(id);
    e->eval = expr_eval_fn;
    return e;
}


/*********************************************************************
 *                                                                   *
 * Static evaluation function                                        *
 *                                                                   *
 *********************************************************************/

static struct value *
expr_eval_fn(struct expr * e) {
    /* Get the function address */
    struct statement * branch = symbol_proc_call(value_string_peek(e->subs[1]->val));
    if ( !branch ) {
        error_set(ERR_NO_SUCH_FN_PROC);
        return NULL;
    }
    
    /* Push a new stack frame and pass the arguments */
    const enum basic_error err = pass_arguments(branch->e[0], e->subs[0]);
    if ( err != ERR_NO_ERROR ) {
        return NULL;
    }

    /* Push the return address onto the stack. This can't be
     * done in the statement function, because the decision
     * to branch is made in this function.
     */
    push_function();

    /* Run the function */
    if ( run_statements(branch) != STATUS_EXIT ) {
        return NULL;
    }

    /* Return the result from the stack - the FN return function pushed it */
    return runtime_stack_pop();
}
