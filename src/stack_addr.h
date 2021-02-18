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

#ifndef PG_BBASIC_INTERNAL_STACK_ADDR_H
#define PG_BBASIC_INTERNAL_STACK_ADDR_H

#include <stddef.h>
#include <stdbool.h>

struct statement;

/* Stack object */
struct stack_addr {
    struct statement ** data;
    size_t top;
    size_t size;
};

/* Stack functions */
bool stack_addr_empty(struct stack_addr * stack);
void stack_addr_free(struct stack_addr * stack);
struct statement * stack_addr_peek(struct stack_addr * stack);
struct statement * stack_addr_pop(struct stack_addr * stack);
void stack_addr_push(struct stack_addr * stack, struct statement * stmt);

#endif  /* PG_BBASIC_INTERNAL_STACK_ADDR_H */
