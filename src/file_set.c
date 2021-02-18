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

#include "file_set.h"
#include "util.h"

/* Adds a file to the set */
void
file_set_add(struct file_set * s, const int fd) {
    struct file_set_node * node = x_malloc(sizeof *node);
    node->fd = fd;
    node->ptr = 0;
    node->next = s->head;
    s->head = node;
}

/* Checks if a set is empty */
bool
file_set_empty(struct file_set *s) {
    return s->head == NULL;
}

/* Finds a file in the set. The caller should not modify
 * or free the returned pointer.
 */
struct file_set_node *
file_set_find(struct file_set * s, const int fd) {
    struct file_set_node * current = s->head;

    while ( current ) {
        if ( current->fd == fd ) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

/* Frees the resources used by a set */
void
file_set_free(struct file_set * s) {
    struct file_set_node * current = s->head;
    while ( current ) {
        struct file_set_node * tmp = current->next;
        free(current);
        current = tmp;
    }
}

/* Checks if a file is a member of the set */
bool
file_set_is_member(struct file_set * s, const int fd) {
    struct file_set_node * current = s->head;
    while ( current ) {
        if ( current->fd == fd ) {
            return true;
        }
        current = current->next;
    }

    return false;
}

/* Removes and returns the first value in the set */
int
file_set_pop(struct file_set *s) {
    if ( !s->head ) {
        ABORT("set empty");
    }

    struct file_set_node * node = s->head;
    s->head = node->next;
    const int n = node->fd;
    free(node);

    return n;
}

/* Removes a file from the set */
void
file_set_remove(struct file_set * s, const int fd) {
    struct file_set_node * current = s->head;
    struct file_set_node * prev = NULL;
    while ( current ) {
        if ( current->fd == fd ) {
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
