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

#ifndef PG_BBASIC_PGCOMMON_UTIL_H
#define PG_BBASIC_PGCOMMON_UTIL_H

#include "internal.h"

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#if HAVE_CONFIG_H
#define ABORT(msg) fprintf(stderr, "%s:%s:%d: %s\n", PACKAGE, __FILE__, __LINE__, msg); \
    fflush(stderr); abort();
#else
#define ABORT(msg) fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, msg); \
    fflush(stderr); abort();
#endif

#if HAVE_CONFIG_H
#define ABORTF(fmt, ...) fprintf(stderr, "%s:%s:%d: ", PACKAGE, __FILE__, __LINE__); \
    fprintf(stderr, fmt, __VA_ARGS__); fputc('\n', stderr); fflush(stderr); abort();
#else
#define ABORTF(fmt, ...) fprintf(stderr, "%s:%d: ", __FILE__, __LINE__); \
    fprintf(stderr, fmt, __VA_ARGS__); fputc('\n', stderr); fflush(stderr); abort();
#endif

void defer_lex_finalize(void);
void * x_malloc(size_t size);
void * x_calloc(size_t nmemb, size_t size);
void * x_realloc(void * ptr, size_t size);
char * x_strdup(const char * s);
FILE * x_fopen(const char * pathname, const char * mode);
void x_fclose(FILE * stream);
void x_atexit(void (*function)(void));
char * x_msprintf(const char * format, ...);

#endif  /*  PG_BBASIC_PGCOMMON_UTIL_H  */
