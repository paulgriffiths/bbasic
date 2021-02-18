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

#ifndef PG_BBASIC_INTERNAL_VALUE_H
#define PG_BBASIC_INTERNAL_VALUE_H

#include <stdint.h>
#include <stdbool.h>

/* Opaque and incomplete struct definition */
struct value;

/* Constructors */
struct value * value_copy(struct value * v);
struct value * value_float_new(const double f);
struct value * value_int_new(const int32_t n);
struct value * value_string_new(const char * s);

/* Getters */
double value_float(struct value * v);
int32_t value_int(struct value * v);
char * value_string_peek(struct value * v);
char * value_string(struct value * v);

/* Type-checking functions */
bool value_is_float(struct value * v);
bool value_is_int(struct value * v);
bool value_is_numeric(struct value * v);
bool value_is_string(struct value * v);
bool value_is_zero(struct value * v);

/* Stringify function */
char * value_to_string(struct value * v, const bool format);

/* List functions */
struct value * value_append(struct value * head, struct value * tail);
struct value * value_next(struct value * v);

/* Destructor */
void value_free(struct value * v);

#endif /* PG_BBASIC_INTERNAL_VALUE_H */
