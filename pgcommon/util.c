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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "util.h"

/* Prototype for function defined by lexer */
int
yylex_destroy(void);

static void
finalize(void) {
    yylex_destroy();
}

/* Adds an exit handler to free lex resources */
void
defer_lex_finalize(void) {
    x_atexit(finalize);
}

/* Wraps malloc and exits on failure */
void *
x_malloc(size_t size) {
    void * p = malloc(size);
    if ( p == NULL ) {
        perror("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    return p;
}

/* Wraps calloc and exits on failure */
void *
x_calloc(size_t nmemb, size_t size) {
    void * p = calloc(nmemb, size);
    if ( p == NULL ) {
        perror("failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    return p;
}

/* Wraps realloc and exits on failure */
void *
x_realloc(void * ptr, size_t size) {
    void * p = realloc(ptr, size);
    if ( p == NULL ) {
        perror("failed to reallocate memory");
        exit(EXIT_FAILURE);
    }

    return p;
}

/* Wraps strdup and exits on failure */
char *
x_strdup(const char * s) {
    char * p = strdup(s);
    if ( p == NULL ) {
        perror("failed to duplicate string");
        exit(EXIT_FAILURE);
    }

    return p;
}

/* Wraps fopen and exits on failure */
FILE *
x_fopen(const char * pathname, const char * mode) {
    FILE * fp = fopen(pathname, mode);
    if ( fp == NULL ) {
        perror(pathname);
        exit(EXIT_FAILURE);
    }

    return fp;
}

/* Wraps fclose and exits on failure */
void
x_fclose(FILE * stream) {
    if ( fclose(stream) != 0 ) {
        perror("failed to close");
        exit(EXIT_FAILURE);
    }
}

/* Wraps atexit and exits on failure */
void
x_atexit(void (*function)(void)) {
    if ( atexit(function) != 0 ) {
        perror("atexit failed");
        exit(EXIT_FAILURE);
    }
}

/* Returns a dynamically allocated string populated by sprintf, and exits on
 * failure. The caller is responsible for freeing the returned string.
 */
char *
x_msprintf(const char * format, ...) {
    va_list ap;

    va_start(ap, format);
    int size = vsnprintf(NULL, 0, format, ap);
    va_end(ap);
    if ( size < 0 ) {
        fprintf(stderr, "vsnprintf failed to write\n");
        exit(EXIT_FAILURE);
    }

    size++; /* For the terminating null */

    char * buffer = x_malloc(size);
    va_start(ap, format);
    int needs = vsnprintf(buffer, size, format, ap);
    va_end(ap);
    if ( needs < 0 || needs >= size ) {
        fprintf(stderr, "vsnprintf failed to write\n");
        exit(EXIT_FAILURE);
    }

    return buffer;
}
