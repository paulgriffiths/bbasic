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

#ifndef PG_BBASIC_INTERNAL_FILE_SET_H
#define PG_BBASIC_INTERNAL_FILE_SET_H

#include <stdbool.h>

struct file_set {
    struct file_set_node * head;
};

struct file_set_node {
    int fd;
    int ptr;
    struct file_set_node * next;
};

void file_set_add(struct file_set * s, const int fd);
bool file_set_empty(struct file_set * s);
struct file_set_node * file_set_find(struct file_set *s, const int fd);
void file_set_free(struct file_set * s);
bool file_set_is_member(struct file_set * s, const int fd);
int file_set_pop(struct file_set *s);
void file_set_remove(struct file_set * s, const int fd);

#endif  /* PG_BBASIC_INTERNAL_FILE_SET_H */
