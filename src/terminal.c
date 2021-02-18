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

#include <signal.h>
#include <stdio.h>
#include <errno.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_TERMIOS_H
#include <termios.h>
#endif

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "terminal.h"
#include "runtime.h"

/* Terminal state values */
enum tty_state {
    TTY_RESET,
    TTY_RAW,
    TTY_CBREAK,
};

/* Static function declaration */
static int tty_cbreak(void);

/* Saved terminal state */
static struct termios save_termios;

/* Terminal state */
static volatile sig_atomic_t tty_state = TTY_RESET;

/* Gets a single character from stdin without echo. hsecs
 * represents the maximum time to wait, in 1/100 of a second.
 * If hsecs if negative, wait indefinitely.
 *
 * Returns EOF on error, or 0 if the timeout passed without input.
 */
int
get_char(int hsecs) {
    /* Set the timeout, if requested */
    struct timeval tv;
    struct timeval * tvptr = NULL;
    if ( hsecs >= 0 ) {
        tv.tv_sec = hsecs / 100;
        tv.tv_usec = (hsecs % 100) * 10000;
        tvptr = &tv;
    }

    /* Create an fd_set consisting solely of stdin */
    fd_set readfs;
    FD_ZERO(&readfs);
    FD_SET(STDIN_FILENO, &readfs);

    /* Switch to cbreak mode */
    if ( tty_cbreak() == -1 ) {
        return EOF;
    }

    /* Wait for input */
    int status = select(STDIN_FILENO+1, &readfs, NULL, NULL, tvptr);
    if ( status == -1 ) {
        if ( errno == EINTR ) {
            error_set(ERR_ESCAPE);
        }

        tty_reset();

        return EOF;
    } else if ( status == 0 ) {
        /* Timeout passed without input */
        tty_reset();

        return 0;
    }

    /* Input is ready, so read one character */
    char c;
    status = read(STDIN_FILENO, &c, 1);
    if ( status != 1 ) {
        if ( errno == EINTR ) {
            error_set(ERR_ESCAPE);
        }

        tty_reset();

        return EOF;
    }

    /* Reset the terminal to canonical mode, and return */
    tty_reset();

    return (int) c;
}

/* Puts the terminal back into canonical mode */
int
tty_reset(void) {
    /* Do nothing if we're already in canonical mode */
    if ( tty_state == TTY_RESET ) {
        return 0;
    }

    if ( tcsetattr(STDIN_FILENO, TCSAFLUSH, &save_termios) == -1 ) {
        return -1;
    }

    tty_state = TTY_RESET;

    return 0;
}

/* Exit handler for calling tty_reset(), if necessary */
void
tty_atexit(void) {
    if ( tty_state != TTY_RESET ) {
        tty_reset();
    }
}

/* Puts the terminal in cbreak mode */
static int
tty_cbreak(void) {
    /* Fail if we're not in a RESET state to begin with */
    if ( tty_state != TTY_RESET ) {
        errno = EINVAL;

        return -1;
    }

    /* Save the existing terminal state */
    struct termios buf;
    if ( tcgetattr(STDIN_FILENO, &buf) == -1 ) {
        return -1;
    }
    save_termios = buf;

    /* Set the terminal to cbreak mode */
    buf.c_lflag &= ~(ECHO | ICANON);
    buf.c_cc[VMIN] = 1;
    buf.c_cc[VTIME] = 0;

    if ( tcsetattr(STDIN_FILENO, TCSAFLUSH, &buf) == -1 ) {
        return -1;
    }

    /* Verify the terminal attributes were appropriately set */
    if ( tcgetattr(STDIN_FILENO, &buf) == -1 ) {
        int err = errno;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &save_termios);
        errno = err;

        return -1;
    }

    if ( (buf.c_lflag & (ECHO | ICANON)) || buf.c_cc[VMIN] != 1 || buf.c_cc[VTIME] != 0 ) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &save_termios);
        errno = EINVAL;

        return -1;
    }

    /* Set the saved state and return */
    tty_state = TTY_CBREAK;

    return 0;
}
