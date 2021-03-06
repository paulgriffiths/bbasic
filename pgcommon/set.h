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

#ifndef PG_BBASIC_PGCOMMON_SET_H
#define PG_BBASIC_PGCOMMON_SET_H

#include <stdbool.h>

struct set_int {
    struct set_int_node * head;
};

void set_int_add(struct set_int * s, const int n);
bool set_int_empty(struct set_int * s);
void set_int_free(struct set_int * s);
bool set_int_is_member(struct set_int * s, const int n);
int set_int_pop(struct set_int *s);
void set_int_remove(struct set_int * s, const int n);

#endif  /* PG_BBASIC_PGCOMMON_SET_H */
