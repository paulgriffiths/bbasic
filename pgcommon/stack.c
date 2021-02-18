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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "stack.h"
#include "util.h"

struct stack_int_node {
    int n;
    struct stack_int_node * next;
};

/* Checks if a stack is empty */
bool
stack_int_empty(struct stack_int *s) {
    return s->head == NULL;
}

/* Pushes a value onto the stack */
void
stack_int_push(struct stack_int * s, const int n) {
    struct stack_int_node * node = x_malloc(sizeof *node);
    node->n = n;
    node->next = s->head;
    s->head = node;
}

/* Returns the top value without popping it */
int
stack_int_peek(struct stack_int *s) {
    if ( !s->head ) {
        ABORT("stack empty");
    }

    return s->head->n;
}

/* Pops a value from the stack */
int
stack_int_pop(struct stack_int * s) {
    if ( !s->head ) {
        ABORT("stack empty");
    }

    int n = s->head->n;
    struct stack_int_node * node = s->head;
    s->head = node->next;
    free(node);

    return n;
}

/* Frees the resources used by a stack */
void
stack_int_free(struct stack_int * s) {
    struct stack_int_node * current = s->head;
    while ( current ) {
        struct stack_int_node * tmp = current->next;
        free(current);
        current = tmp;
    }
}
