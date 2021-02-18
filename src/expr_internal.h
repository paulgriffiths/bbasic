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

#ifndef PG_BBASIC_INTERNAL_EXPR_INTERNAL_H
#define PG_BBASIC_INTERNAL_EXPR_INTERNAL_H

#include "value.h"

/* Expression types */
enum expr_type {
    EXPR_ARRAY = 1,
    EXPR_CONSTANT,
    EXPR_FN,
    EXPR_FUNC_ABS,
    EXPR_FUNC_ACS,
    EXPR_FUNC_ASC,
    EXPR_FUNC_ASN,
    EXPR_FUNC_ATN,
    EXPR_FUNC_BGET,
    EXPR_FUNC_CHRS,
    EXPR_FUNC_COS,
    EXPR_FUNC_DEG,
    EXPR_FUNC_EOF,
    EXPR_FUNC_ERL,
    EXPR_FUNC_ERR,
    EXPR_FUNC_EXP,
    EXPR_FUNC_EXT,
    EXPR_FUNC_GET,
    EXPR_FUNC_GETS,
    EXPR_FUNC_INKEY,
    EXPR_FUNC_INKEYS,
    EXPR_FUNC_INSTR,
    EXPR_FUNC_INT,
    EXPR_FUNC_LEFTS,
    EXPR_FUNC_LEN,
    EXPR_FUNC_LN,
    EXPR_FUNC_LOG,
    EXPR_FUNC_MIDS,
    EXPR_FUNC_OPENIN,
    EXPR_FUNC_OPENOUT,
    EXPR_FUNC_OPENUP,
    EXPR_FUNC_PTR,
    EXPR_FUNC_RAD,
    EXPR_FUNC_RIGHTS,
    EXPR_FUNC_RND,
    EXPR_FUNC_SGN,
    EXPR_FUNC_SIN,
    EXPR_FUNC_SPC,
    EXPR_FUNC_SQR,
    EXPR_FUNC_STRINGS,
    EXPR_FUNC_STRS,
    EXPR_FUNC_TAN,
    EXPR_FUNC_VAL,
    EXPR_OP_ADD,
    EXPR_OP_AND,
    EXPR_OP_DIV,
    EXPR_OP_EOR,
    EXPR_OP_EQ,
    EXPR_OP_EXP,
    EXPR_OP_GT,
    EXPR_OP_GTE,
    EXPR_OP_IDIV,
    EXPR_OP_LT,
    EXPR_OP_LTE,
    EXPR_OP_MOD,
    EXPR_OP_MUL,
    EXPR_OP_NEQ,
    EXPR_OP_NOT,
    EXPR_OP_OR,
    EXPR_OP_SUB,
    EXPR_OP_UMINUS,
    EXPR_VARIABLE
};

#define EXPR_NUM_SUBS (3)

/* Tagged union expression object */
struct expr {
    enum expr_type type;

    struct value * val;
    struct expr * subs[EXPR_NUM_SUBS];
    struct expr * next;

    struct value * (*eval)(struct expr *);
};

struct expr * expr_new(enum expr_type t);

#include "expr.h"

#endif /* PG_BBASIC_INTERNAL_EXPR_INTERNAL_H */
