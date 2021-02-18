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

#ifndef PG_BBASIC_PGCOMMON_STACK_H
#define PG_BBASIC_PGCOMMON_STACK_H

#include <stdbool.h>

struct stack_int {
    struct stack_int_node * head;
};

bool
stack_int_empty(struct stack_int * s);

void
stack_int_free(struct stack_int * s);

int
stack_int_peek(struct stack_int *s);

void
stack_int_push(struct stack_int * s, const int n);

int
stack_int_pop(struct stack_int * s);

#endif  /* PG_BBASIC_PGCOMMON_STACK_H */
