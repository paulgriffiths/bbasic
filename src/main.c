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
#include <signal.h>

#include "parser.h"
#include "util.h"
#include "options.h"
#include "runtime.h"
#include "symbols.h"
#include "yydecls.h"
#include "terminal.h"

/* Signal handler */
void
handler(int signum) {
    (void) signum;

    /* Reset the terminal state if necessary */
    tty_reset();

    /* Set the interrupt flag */
    interrupt = 1;
}

/* Main function */
int main(int argc, char ** argv) {
    YY_BUFFER_STATE buffer = NULL;
    FILE * infile = NULL;

    process_options(argc, argv);
    defer_lex_finalize();

    /* Get input depending on command line */
    if ( input_inline ) {
        /* Process inline input if provided */
        buffer = yy_scan_buffer(input_inline, strlen(input_inline)+2);
    } else if ( input_filename ) {
        /* Otherwise process input file */
        infile = x_fopen(input_filename, "r");
        yyrestart(infile);
    } else {
        fprintf(stderr, "%s: no input provided\n", PACKAGE);
        return EXIT_FAILURE;
    }

    /* Save status to exit after freeing resources on error */
    int status = yyparse();

    /* Free input depending on input method */
    if ( buffer ) {
        yy_delete_buffer(buffer);
    } else if ( infile ) {
        x_fclose(infile);
    }

    if ( status != 0 ) {
        return EXIT_FAILURE;
    }

    /* Catch SIGINT, to print an Escape error message, and to
     * clean up the terminal, if necessary.
     */
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_handler = handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if ( sigaction(SIGINT, &act, NULL) == -1 ) {
        perror("sigaction failed");
        exit(EXIT_FAILURE);
    }

    /* Run program */
    symbol_table_init();
    x_atexit(tty_atexit);

    status = program_run();

    if ( status != STATUS_OK ) {
        if ( status >= 0 ) {
            return status;
        }

        if ( error_is_set() ) {
            status = error_code();
            if ( status > 0 ) {
                return status;
            }
        }

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
