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

#include "set.h"
#include "util.h"

struct set_int_node {
    int n;
    struct set_int_node * next;
};

/* Adds a value to the set */
void
set_int_add(struct set_int * s, const int n) {
    struct set_int_node * node = x_malloc(sizeof *node);
    node->n = n;
    node->next = s->head;
    s->head = node;
}

/* Checks if a set is empty */
bool
set_int_empty(struct set_int *s) {
    return s->head == NULL;
}

/* Frees the resources used by a set */
void
set_int_free(struct set_int * s) {
    struct set_int_node * current = s->head;
    while ( current ) {
        struct set_int_node * tmp = current->next;
        free(current);
        current = tmp;
    }
}

/* Checks if a value is a member of the set */
bool
set_int_is_member(struct set_int * s, const int n) {
    struct set_int_node * current = s->head;
    while ( current ) {
        if ( current->n == n ) {
            return true;
        }
        current = current->next;
    }

    return false;
}

/* Removes and returns the first value in the set */
int
set_int_pop(struct set_int *s) {
    if ( !s->head ) {
        ABORT("set empty");
    }

    struct set_int_node * node = s->head;
    s->head = node->next;
    const int n = node->n;
    free(node);

    return n;
}

/* Removes a value from the set */
void
set_int_remove(struct set_int * s, const int n) {
    struct set_int_node * current = s->head;
    struct set_int_node * prev = NULL;
    while ( current ) {
        if ( current->n == n ) {
            if ( !prev ) {
                s->head = current->next;
            } else {
                prev->next = current->next;
            }
            free(current);
            break;
        }
        current = current->next;
    }
}
