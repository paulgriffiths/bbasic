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

#include "internal.h"

#include <stdlib.h>
#include <time.h>

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "rand.h"

/* Seeds the random number generator, using n as an input */
void
seed_prng(int n) {
    unsigned seed = (unsigned)time(NULL) + n;
#ifdef HAVE_GETPID
    /* Add some per-process randomness, if available */
    seed += getpid();
#endif
    srand(seed);
}

/* Gets a random integer between 1 and n, inclusive
 * NOTE: the quality of random numbers generated by this function
 * is poor, and it's possible to do a lot better. However, the PRNG
 * in BBC BASIC II is at least as poor, so we won't worry about it,
 * and we'll risk the ire of the random number pedants.
 */
int
get_random(int n) {
    return (int)(n * (rand() / (RAND_MAX + 1.0))) + 1;
}
