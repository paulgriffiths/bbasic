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

#ifndef PG_BBASIC_PGCOMMON_HASH_H
#define PG_BBASIC_PGCOMMON_HASH_H

#include "internal.h"

#include <stddef.h>
#include <stdint.h>

size_t djb2hash(const char * str, const int num_buckets);
size_t int_hash(const int32_t n, const int num_buckets);
size_t pointer_hash(const void * p, const int num_buckets);

#endif  /*  PG_BBASIC_PGCOMMON_HASH_H  */
