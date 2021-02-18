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

/* A set of memory addresses, intended for use by applications where
 * it is desired to record whether an operation has previously been
 * performed on a given address, so that graph-like structures can
 * be easily traversed.
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "addr_set.h"
#include "hash.h"
#include "util.h"

#define NUM_BUCKETS (257)

/* An entry in the set */
struct entry {
    void * addr;
    struct entry * next;
};

struct addr_set {
    struct entry * buckets[NUM_BUCKETS];
};

/* Creates a new set */
struct addr_set *
addr_set_new(void) {
    return x_calloc(1, sizeof(struct addr_set));
}

/* Adds an address to the set */
void
addr_set_add(struct addr_set * set, void * addr) {
    struct entry * ref = x_malloc(sizeof *ref);
    ref->addr = addr;
    ref->next = NULL;

    const size_t hash = pointer_hash(addr, NUM_BUCKETS);
    struct entry * current = set->buckets[hash];
    if ( current == NULL ) {
        /* Bucket does not exist, so create it */
        set->buckets[hash] = ref;
    } else {
        /* Bucket exists, so append */
        while ( true ) {
            if ( current->addr == ref->addr ) {
                /* Address is already in the set */
                return;
            }

            if ( !current->next ) {
                current->next = ref;
                break;
            }

            current = current->next;
        }
    }
}

/* Tests an address for membership in the set */
bool
addr_set_is_member(struct addr_set * set, void * addr) {
    const size_t hash = pointer_hash(addr, NUM_BUCKETS);
    struct entry * current = set->buckets[hash];
    while ( current ) {
        if ( current->addr == addr ) {
            return true;
        }
        current = current->next;
    }

    return false;
}

/* Frees all resources associated with a set */
void
addr_set_free(struct addr_set * set) {
    for ( int i = 0; i < NUM_BUCKETS; i++ ) {
        if ( set->buckets[i] ) {
            struct entry * current = set->buckets[i];
            while ( current ) {
                struct entry * tmp = current->next;
                free(current);
                current = tmp;
            }
        }
    }
    free(set);
}
