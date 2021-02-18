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

#ifndef PG_BBASIC_INTERNAL_EXPR_VALUE_H
#define PG_BBASIC_INTERNAL_EXPR_VALUE_H

#include <stdint.h>

#include "value.h"

/* Opaque and incomplete struct definition */
struct expr;

/* Constructors */
struct expr * expr_array_new(const char * id, struct expr * indices);
struct expr * expr_constant_new(struct value * v);
struct expr * expr_float_new(const double d);
struct expr * expr_int_new(const int32_t n);
struct expr * expr_string_new(const char * s);
struct expr * expr_variable_new(const char * id);

/* Values */
struct expr * expr_indices_peek(struct expr * e);
bool expr_is_array(struct expr * e);
bool expr_is_constant(struct expr * e);
bool expr_is_variable(struct expr * e);
char * expr_id_peek(struct expr * e);

#endif  /* PG_BBASIC_INTERNAL_EXPR_VALUE_H */
