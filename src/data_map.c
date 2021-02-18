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

/* A hash map between program line numbers and their entries in the
 * data items list, to enable the RESTORE keyword.
 */

#include <stdlib.h>
#include <stdint.h>

#include "data_map.h"
#include "hash.h"
#include "runtime.h"
#include "util.h"

#define NUM_BUCKETS (257)

/* An entry in the map */
struct entry {
    int32_t line;
    struct value * data;
    struct entry * next;
};

/* The data map */
static struct table {
    struct entry * buckets[NUM_BUCKETS];
} table;

/* Static function declarations */
static void entry_free(struct entry * ref);
static void table_free(void);

/* Adds a data pointer to the data table */
int
data_map_add(const int line, struct value * data) {
    struct entry * ref = x_malloc(sizeof *ref);
    ref->line = line;
    ref->data = data;
    ref->next = NULL;

    const size_t hash = int_hash(line, NUM_BUCKETS);
    struct entry * current = table.buckets[hash];
    if ( current == NULL ) {
        /* Bucket does not exist, so create it */
        table.buckets[hash] = ref;
    } else {
        /* Bucket exists, so append */
        while ( true ) {
            if ( current->line == ref->line ) {
                /* There is a duplicate line number */
                error_set(ERR_BAD_PROGRAM);
                free(ref);
                data_map_free();
                return STATUS_ERROR;
            }

            if ( !current->next ) {
                current->next = ref;
                break;
            }

            current = current->next;
        }
    }

    return STATUS_OK;
}

/* Looks for a data pointer in the data map */
struct value *
data_map_find(const int line) {
    struct value * data = NULL;

    const size_t hash = int_hash(line, NUM_BUCKETS);
    struct entry * current = table.buckets[hash];
    while ( current ) {
        if ( current->line == line ) {
            data = current->data;
            break;
        }
        current = current->next;
    }

    if ( data == NULL ) {
        error_set(ERR_NO_SUCH_LINE);
    }

    return data;
}

/* Frees all resources associated with the data map */
void
data_map_free(void) {
    for ( int i = 0; i < NUM_BUCKETS; i++ ) {
        if ( table.buckets[i] ) {
            struct entry * current = table.buckets[i];
            while ( current ) {
                struct entry * tmp = current->next;
                free(current);
                current = tmp;
            }
            table.buckets[i] = NULL; /* So we could reuse it */
        }
    }
}
