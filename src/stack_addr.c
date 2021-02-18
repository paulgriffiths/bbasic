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

/* A return address stack */

#include <stdbool.h>

#include "stack_addr.h"
#include "util.h"

#define INITIAL_STACK_SIZE (16)

/* Returns true if the stack is empty */
bool
stack_addr_empty(struct stack_addr * stack) {
    return stack->top == 0;
}

/* Frees resources associated with a stack */
void
stack_addr_free(struct stack_addr * stack) {
    free(stack->data);
    stack->data = NULL;
    stack->top = 0;
    stack->size = 0;
}

/* Peeks at top stack address without popping it */
struct statement *
stack_addr_peek(struct stack_addr * stack) {
    if ( stack->top == 0 ) {
        ABORT("stack empty");
    }
    return stack->data[stack->top-1];
}

/* Pops the top stack address */
struct statement *
stack_addr_pop(struct stack_addr * stack) {
    if ( stack->top == 0 ) {
        ABORT("stack empty");
    }
    return stack->data[--stack->top];
}

/* Pushes an address onto the stack */
void
stack_addr_push(struct stack_addr * stack, struct statement * addr) {
    if ( stack->size == 0 ) {
        stack->size = INITIAL_STACK_SIZE;
        stack->data = x_malloc(sizeof *stack->data * stack->size);
    } else if ( stack->top == stack->size ) {
        stack->size *= 2;
        stack->data = x_realloc(stack->data, sizeof *stack->data * stack->size);
    }

    stack->data[stack->top++] = addr;
}
