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

#include <stdlib.h>
#include <stdint.h>

/* Computes a hash value for a string */
size_t
djb2hash(const char * str, const int num_buckets)
{
    size_t hash = 5381;
    int c;

    while ( (c = *str++) ) {
        hash = ((hash << 5) + hash) + c;
    }

    return hash % num_buckets;
}

/* Computes a hash value for an integer */
size_t
int_hash(const int32_t n, const int num_buckets) {
    uint32_t u = (uint32_t) n;
    u = ((u >> 16) ^ u) * 0x45d9f3b;
    u = ((u >> 16) ^ u) * 0x45d9f3b;
    u = (u >> 16) ^ u;

    return u % num_buckets;
}

/* Computes a hash value for a pointer to void */
size_t
pointer_hash(const void * p, const int num_buckets) {
    uintptr_t u = (uintptr_t) p;
    u += 1ULL;
    u ^= u >> 33ULL;
    u *= 0xff51afd7ed558ccdULL;
    u ^= u >> 33ULL;
    u *= 0xc4ceb9fe1a85ec53ULL;
    u ^= u >> 33ULL;

    return u % num_buckets;
}
