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

#ifndef PG_BBASIC_INTERNAL_ADDR_SET_H
#define PG_BBASIC_INTERNAL_ADDR_SET_H

#include <stdbool.h>

struct addr_set;

struct addr_set * addr_set_new(void);
void addr_set_add(struct addr_set * set, void * addr);
bool addr_set_is_member(struct addr_set * set, void * addr);
void addr_set_free(struct addr_set * set);

#endif  /* PG_BBASIC_INTERNAL_ADDR_SET_H */
