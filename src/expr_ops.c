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

/* Functions and types for operator expressions */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <errno.h>

#include "expr_internal.h"
#include "runtime.h"
#include "util.h"

/* Static function declarations */
static struct expr * expr_op_unary_new(struct expr * e, enum expr_type t);
static struct expr * expr_op_binary_new(struct expr * e,
        struct expr * f, enum expr_type t);

static struct value * expr_eval_op_binary_arith(struct expr * e);
static struct value * expr_eval_op_binary_comp(struct expr * e);
static struct value * expr_eval_op_unary_minus(struct expr * e);
static struct value * expr_eval_op_unary_not(struct expr * e);


/*********************************************************************
 *                                                                   *
 * Constructor functions                                             *
 *                                                                   *
 *********************************************************************/

/* Creates a new binary addition operator */
struct expr *
expr_op_add_new(struct expr * l, struct expr * r) {
    struct expr * e = expr_op_binary_new(l, r, EXPR_OP_ADD);
    e->eval = expr_eval_op_binary_arith;
    return e;
}

/* Creates a new binary AND operator */
struct expr *
expr_op_and_new(struct expr * l, struct expr * r) {
    struct expr * e = expr_op_binary_new(l, r, EXPR_OP_AND);
    e->eval = expr_eval_op_binary_arith;
    return e;
}

/* Creates a new binary division operator */
struct expr *
expr_op_div_new(struct expr * l, struct expr * r) {
    struct expr * e = expr_op_binary_new(l, r, EXPR_OP_DIV);
    e->eval = expr_eval_op_binary_arith;
    return e;
}

/* Creates a new binary donkey operator */
struct expr *
expr_op_eor_new(struct expr * l, struct expr * r) {
    struct expr * e = expr_op_binary_new(l, r, EXPR_OP_EOR);
    e->eval = expr_eval_op_binary_arith;
    return e;
}

/* Creates a new binary equals operator */
struct expr *
expr_op_eq_new(struct expr * l, struct expr * r) {
    struct expr * e = expr_op_binary_new(l, r, EXPR_OP_EQ);
    e->eval = expr_eval_op_binary_comp;
    return e;
}

/* Creates a new binary exponentiation operator */
struct expr *
expr_op_exp_new(struct expr * l, struct expr * r) {
    struct expr * e = expr_op_binary_new(l, r, EXPR_OP_EXP);
    e->eval = expr_eval_op_binary_arith;
    return e;
}

/* Creates a new binary greater-than operator */
struct expr *
expr_op_gt_new(struct expr * l, struct expr * r) {
    struct expr * e = expr_op_binary_new(l, r, EXPR_OP_GT);
    e->eval = expr_eval_op_binary_comp;
    return e;
}

/* Creates a new binary greater-than-or-equals operator */
struct expr *
expr_op_gte_new(struct expr * l, struct expr * r) {
    struct expr * e = expr_op_binary_new(l, r, EXPR_OP_GTE);
    e->eval = expr_eval_op_binary_comp;
    return e;
}

/* Creates a new binary DIV operator */
struct expr *
expr_op_idiv_new(struct expr * l, struct expr * r) {
    struct expr * e = expr_op_binary_new(l, r, EXPR_OP_IDIV);
    e->eval = expr_eval_op_binary_arith;
    return e;
}

/* Creates a new binary less-than operator */
struct expr *
expr_op_lt_new(struct expr * l, struct expr * r) {
    struct expr * e = expr_op_binary_new(l, r, EXPR_OP_LT);
    e->eval = expr_eval_op_binary_comp;
    return e;
}

/* Creates a new binary less-than-or-equals operator */
struct expr *
expr_op_lte_new(struct expr * l, struct expr * r) {
    struct expr * e = expr_op_binary_new(l, r, EXPR_OP_LTE);
    e->eval = expr_eval_op_binary_comp;
    return e;
}

/* Creates a new binary MOD operator */
struct expr *
expr_op_mod_new(struct expr * l, struct expr * r) {
    struct expr * e = expr_op_binary_new(l, r, EXPR_OP_MOD);
    e->eval = expr_eval_op_binary_arith;
    return e;
}

/* Creates a new binary multiplication operator */
struct expr *
expr_op_mul_new(struct expr * l, struct expr * r) {
    struct expr * e = expr_op_binary_new(l, r, EXPR_OP_MUL);
    e->eval = expr_eval_op_binary_arith;
    return e;
}

/* Creates a new binary not-equals operator */
struct expr *
expr_op_neq_new(struct expr * l, struct expr * r) {
    struct expr * e = expr_op_binary_new(l, r, EXPR_OP_NEQ);
    e->eval = expr_eval_op_binary_comp;
    return e;
}

/* Creates a new unary NOT operator */
struct expr *
expr_op_not_new(struct expr * e) {
    struct expr * expr = expr_op_unary_new(e, EXPR_OP_NOT);
    expr->eval = expr_eval_op_unary_not;
    return expr;
}

/* Creates a new binary OR operator */
struct expr *
expr_op_or_new(struct expr * l, struct expr * r) {
    struct expr * e = expr_op_binary_new(l, r, EXPR_OP_OR);
    e->eval = expr_eval_op_binary_arith;
    return e;
}

/* Creates a new binary subtraction operator */
struct expr *
expr_op_sub_new(struct expr * l, struct expr * r) {
    struct expr * e = expr_op_binary_new(l, r, EXPR_OP_SUB);
    e->eval = expr_eval_op_binary_arith;
    return e;
}

/* Creates a new unary negative operator */
struct expr *
expr_op_uminus_new(struct expr * e) {
    struct expr * expr = expr_op_unary_new(e, EXPR_OP_UMINUS);
    expr->eval = expr_eval_op_unary_minus;
    return expr;
}


/*********************************************************************
 *                                                                   *
 * Static common sub-constructor functions                           *
 *                                                                   *
 *********************************************************************/

/* Common sub-constructor for a unary operator */
static struct expr *
expr_op_unary_new(struct expr * e, enum expr_type t) {
    struct expr * expr = expr_new(t);
    expr->subs[0] = e;
    return expr;
}

/* Common sub-constructor for a binary operator */
static struct expr *
expr_op_binary_new(struct expr * e, struct expr * f, enum expr_type t) {
    struct expr * expr = expr_new(t);
    expr->subs[0] = e;
    expr->subs[1] = f;
    return expr;
}


/*********************************************************************
 *                                                                   *
 * Static evaluation functions                                       *
 *                                                                   *
 *********************************************************************/

/* Evaluates a binary arithmetic operator */
static struct value *
expr_eval_op_binary_arith(struct expr * e) {
    struct value * l = expr_eval(e->subs[0]);
    struct value * r = expr_eval(e->subs[1]);
    if ( !l || !r ) {
        value_free(l);
        value_free(r);
        return NULL;
    }

    struct value * result = NULL;

    if ( value_is_string(l) && value_is_string(r) && e->type == EXPR_OP_ADD ) {
        /* Perform string concatenation */
        const char * left = value_string_peek(l);
        const char * right = value_string_peek(r);

        char * s = x_malloc(strlen(left) + strlen(right) + 1);
        strcpy(s, left);
        strcat(s, right);
        result = value_string_new(s);
        free(s);
    } else if ( value_is_int(l) && value_is_int(r) ) {
        /* If both operands are integers, then make the result an
         * integer, unless:
         *  - it's a division operation where the divisor
         *    doesn't exactly divide the dividend; or
         *  - it's an exponentiation operation
         */
        const int32_t left = value_int(l);
        const int32_t right = value_int(r);

        switch ( e->type ) {
            case EXPR_OP_ADD:
                result = value_int_new(left + right);
                break;

            case EXPR_OP_AND:
                result = value_int_new(left & right);
                break;

            case EXPR_OP_DIV:
                if ( value_is_zero(r) ) {
                    error_set(ERR_DIVIDE_ZERO);
                    break;
                }

                if ( left % right == 0 ) {
                    result = value_int_new(left / right);
                } else {
                    result = value_float_new((float) left / right);
                }
                break;

            case EXPR_OP_EOR:
                result = value_int_new(left ^ right);
                break;

            case EXPR_OP_EXP:
                errno = 0;
                const double d = pow(left, right);
                if ( (errno == EDOM) || (errno == ERANGE) ) {
                    error_set(ERR_LOG_RANGE);
                    break;
                } else {
                    result = value_float_new(d);
                }

                break;

            case EXPR_OP_IDIV:
                if ( value_is_zero(r) ) {
                    error_set(ERR_DIVIDE_ZERO);
                    break;
                }

                result = value_int_new(left / right);
                break;

            case EXPR_OP_MOD:
                if ( value_is_zero(r) ) {
                    error_set(ERR_DIVIDE_ZERO);
                    break;
                }

                result = value_int_new(left % right);
                break;

            case EXPR_OP_MUL:
                result = value_int_new(left * right);
                break;

            case EXPR_OP_OR:
                result = value_int_new(left | right);
                break;

            case EXPR_OP_SUB:
                result = value_int_new(left - right);
                break;

            default:
                ABORTF("unexpected expression type: %d", e->type);
        }
    } else if ( value_is_numeric(l) && value_is_numeric(r) ) {
        /* If at least one of the operands is a float, the type
         * of the whole expresssion is a float, unless it's an
         * integer division, modulo, or boolean operation, where
         * the type is inherently integral.
         */
        const double left = value_float(l);
        const double right = value_float(r);
        const int32_t ln = value_int(l);
        const int32_t rn = value_int(r);

        switch ( e->type ) {
            case EXPR_OP_ADD:
                result = value_float_new(left + right);
                break;

            case EXPR_OP_AND:
                result = value_int_new(ln & rn);
                break;

            case EXPR_OP_DIV:
                if ( value_is_zero(r) ) {
                    error_set(ERR_DIVIDE_ZERO);
                    break;
                }

                result = value_float_new(left / right);
                break;

            case EXPR_OP_EOR:
                result = value_int_new(ln ^ rn);
                break;

            case EXPR_OP_EXP:
                errno = 0;
                const double d = pow(left, right);
                if ( (errno == EDOM) || (errno == ERANGE) ) {
                    error_set(ERR_LOG_RANGE);
                    break;
                } else {
                    result = value_float_new(d);
                }

                break;

            case EXPR_OP_IDIV:
                if ( value_is_zero(r) ) {
                    error_set(ERR_DIVIDE_ZERO);
                    break;
                }

                result = value_int_new(ln / rn);
                break;

            case EXPR_OP_MOD:
                if ( value_is_zero(r) ) {
                    error_set(ERR_DIVIDE_ZERO);
                    break;
                }

                result = value_int_new(ln % rn);
                break;

            case EXPR_OP_MUL:
                result = value_float_new(left * right);
                break;

            case EXPR_OP_OR:
                result = value_int_new(ln | rn);
                break;

            case EXPR_OP_SUB:
                result = value_float_new(left - right);
                break;

            default:
                ABORTF("unexpected expression type: %d", e->type);
        }
    } else {
        error_set(ERR_TYPE_MISMATCH);
    }

    value_free(l);
    value_free(r);

    return result;
}

/* Evaluates a binary comparison operator */
static struct value *
expr_eval_op_binary_comp(struct expr * e) {
    struct value * l = expr_eval(e->subs[0]);
    struct value * r = expr_eval(e->subs[1]);
    if ( !l || !r ) {
        value_free(l);
        value_free(r);
        return NULL;
    }

    /* Calculate equals and less based on expression type -
     * we only need these two values to compute all six results
     */
    bool equals;
    bool less;

    if ( value_is_numeric(l) && value_is_numeric(r) ) {
        equals = (value_float(l) == value_float(r));
        less = (value_float(l) < value_float(r));
    } else if ( value_is_string(l) && value_is_string(r) ) {
        int c = strcmp(value_string_peek(l), value_string_peek(r));
        equals = (c == 0);
        less = (c < 0);
    } else {
        error_set(ERR_TYPE_MISMATCH);
        value_free(l);
        value_free(r);
        return NULL;
    }

    /* Compute result according to specific operator */
    struct value * result = NULL;

    switch ( e->type ) {
        case EXPR_OP_EQ:
            result = value_int_new(equals ? -1 : 0);
            break;

        case EXPR_OP_GT:
            result = value_int_new((less || equals) ? 0 : -1);
            break;

        case EXPR_OP_GTE:
            result = value_int_new(less ? 0 : -1);
            break;

        case EXPR_OP_LT:
            result = value_int_new(less ? -1 : 0);
            break;

        case EXPR_OP_LTE:
            result = value_int_new((less || equals) ? -1 : 0);
            break;

        case EXPR_OP_NEQ:
            result = value_int_new(equals ? 0: -1);
            break;

        default:
            ABORTF("unexpected expression type: %d", e->type);
    }

    value_free(l);
    value_free(r);

    return result;
}

/* Evaluates a unary minus operator */
static struct value *
expr_eval_op_unary_minus(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    struct value * r = NULL;

    if ( value_is_float(v) ) {
        r = value_float_new(-value_float(v));
    } else if ( value_is_int(v) ) {
        r = value_int_new(-value_int(v));
    } else {
        error_set(ERR_TYPE_MISMATCH);
    }

    value_free(v);

    return r;
}

/* Evaluates a unary NOT operator */
struct value *
expr_eval_op_unary_not(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    struct value * r = NULL;
    if ( value_is_numeric(v) ) {
        r = value_int_new(~value_int(v));
    } else {
        error_set(ERR_TYPE_MISMATCH);
    }

    value_free(v);

    return r;
}
