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

/* Functions and global variables for processing command line options */

#include "internal.h"

#ifdef HAVE_GETOPT_H
#define _XOPEN_SOURCE
#endif

#ifdef HAVE_GETOPT_LONG
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "options.h"
#include "util.h"

/* Options global variables */
int debug_flag;
char * input_inline;
char * input_filename;

/* Static function declarations */
static void process_cmdline(int, char **);
static void output_help(int, char **);

/* Static options flags */
static int help_flag;
static int version_flag;

/* Processes command line options */
void
process_options(int argc, char ** argv) {
#ifndef HAVE_GETOPT_H
    return;
#else
    process_cmdline(argc, argv);

    if ( version_flag ) {
        puts(PACKAGE_STRING);
        exit(EXIT_SUCCESS);
    } else if ( help_flag ) {
        output_help(argc, argv);
        exit(EXIT_SUCCESS);
    }
#endif
}

/* Frees stored inline input */
static void
free_input_inline(void) {
    free(input_inline);
    input_inline = NULL; /* In case of multiple calls */
}

/* Sets stored inline input. Safe to call multiple times. */
static void
set_input_inline(const char * s) {
    /* If input was previously set, remove it,
     * to preserve only the last
     */
    if ( input_inline ) {
        free(input_inline);
    }

    /* lex expects a doubly null-terminated string */
    input_inline = x_calloc(strlen(optarg)+2, 1);
    strcpy(input_inline, optarg);

    x_atexit(free_input_inline);
}

#ifdef HAVE_GETOPT_H
#ifdef HAVE_GETOPT_LONG

/* Processing with getopt_long */
static void
process_cmdline(int argc, char ** argv) {
    while ( 1 ) {
        struct option long_options[] = {
            {"debug", no_argument, &debug_flag, 1},
            {"help", no_argument, &help_flag, 1},
            {"inline", required_argument, NULL, 0},
            {"version", no_argument, &version_flag, 1},
            {0, 0, 0, 0}
        };

        int option_index = 0;

        int c = getopt_long(argc, argv, "dhi:V", long_options, &option_index);
        if ( c == -1 ) {
            break;
        }

        switch ( c ) {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0) {
                    break;
                }

                /* Process long option arguments */
                switch ( option_index ) {
                    case 2:
                        set_input_inline(optarg);
                        break;
                }

                break;

            case 'd':
                debug_flag = 1;
                break;

            case 'h':
                help_flag = 1;
                break;

            case 'i':
                set_input_inline(optarg);
                break;

            case 'V':
                version_flag = 1;
                break;

            case '?':
                /* getopt_long already printed an error message. */
                exit(EXIT_FAILURE);

            default:
                ABORTF("unhandled option: %d", c);
        }
    }

    /* Retrieve filename, if specified */
    while (optind < argc) {
        if ( input_inline ) {
            fprintf(stderr, "You must provide a filename or inline input, not both\n");
            exit(EXIT_FAILURE);
        }

        if ( input_filename ) {
            fprintf(stderr, "You may only specify one filename\n");
            exit(EXIT_FAILURE);
        }

        input_filename = argv[optind++];
    }
}

static void
output_help(int argc, char ** argv) {
    printf("Usage: %s [OPTIONS] [FILE]\n", PACKAGE);
    printf("Interprets a subset of BBC BASIC II.\n");

    printf("\nMiscellaneous:\n");
    printf("  -d, --debug             enable debug output\n");
    printf("  -h, --help              produce this help message\n");
    printf("  -i, --inline=STRING     provide inline BASIC input\n");
    printf("  -V, --version           report version\n");
}

#else

/* Processing without getopt_long */
static void
process_cmdline(int argc, char ** argv) {
    int c;
    opterr = 0;

    while ( (c = getopt(argc, argv, "dhi:V")) != -1 ) {
        switch ( c ) {
            case 'd':
                debug_flag = 1;
                break;

            case 'h':
                help_flag = 1;
                break;

            case 'i':
                set_input_inline(optarg);
                break;

            case 'V':
                version_flag = 1;
                break;

            case '?':
                if ( optopt == 'i' || optopt == 'o' ) {
                   fprintf (stderr,
                            "Option -%c requires an argument.\n", optopt);
                } else if ( isprint (optopt) ) {
                   fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                } else {
                   fprintf (stderr,
                            "Unknown option character `\\x%x'.\n", optopt);
                }
                exit(EXIT_FAILURE); 

            default:
                ABORTF("unhandled option: %d", c);
        }
    }

    /* Retrieve filename, if specified */
    while (optind < argc) {
        if ( input_inline ) {
            fprintf(stderr, "You must provide a filename or inline input, not both\n");
            exit(EXIT_FAILURE);
        }

        if ( input_filename ) {
            fprintf(stderr, "You may only specify one filename\n");
            exit(EXIT_FAILURE);
        }

        input_filename = argv[optind++];
    }

    return;
}

static void
output_help(int argc, char ** argv) {
    printf("Usage: %s [OPTIONS] [FILE]\n", PACKAGE);
    printf("Interprets a subset of BBC BASIC II.\n");

    printf("\nMiscellaneous:\n");
    printf("  -d,             enable debug output\n");
    printf("  -h,             produce this help message\n");
    printf("  -i=STRING       provide inline BASIC input\n");
    printf("  -V,             report version\n");
}

#endif  /* HAVE_GETOPT_LONG */
#endif  /* HAVE_GETOPT_H */
