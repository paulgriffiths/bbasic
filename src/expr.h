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

#ifndef PG_BBASIC_INTERNAL_EXPR_H
#define PG_BBASIC_INTERNAL_EXPR_H

#include "expr_builtin.h"
#include "expr_fn.h"
#include "expr_ops.h"
#include "expr_value.h"

#include "value.h"

/* Opaque and incomplete struct definition */
struct expr;

/* Expression functions */
struct expr * expr_append(struct expr * head, struct expr * tail);
struct value * expr_eval(struct expr * e);
void expr_free(struct expr * e);
struct expr * expr_next(struct expr * e);

#endif  /* PG_BBASIC_INTERNAL_EXPR_H */
