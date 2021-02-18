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

#ifndef PG_BBASIC_INTERNAL_COLOURS_H
#define PG_BBASIC_INTERNAL_COLOURS_H

#define ANSI_COLOUR_BLACK               "\x1b[30m"
#define ANSI_COLOUR_RED                 "\x1b[31m"
#define ANSI_COLOUR_GREEN               "\x1b[32m"
#define ANSI_COLOUR_YELLOW              "\x1b[33m"
#define ANSI_COLOUR_BLUE                "\x1b[34m"
#define ANSI_COLOUR_MAGENTA             "\x1b[35m"
#define ANSI_COLOUR_CYAN                "\x1b[36m"
#define ANSI_COLOUR_WHITE               "\x1b[37m"
#define ANSI_BACKGROUND_COLOUR_BLACK    "\x1b[40m"
#define ANSI_BACKGROUND_COLOUR_RED      "\x1b[41m"
#define ANSI_BACKGROUND_COLOUR_GREEN    "\x1b[42m"
#define ANSI_BACKGROUND_COLOUR_YELLOW   "\x1b[43m"
#define ANSI_BACKGROUND_COLOUR_BLUE     "\x1b[44m"
#define ANSI_BACKGROUND_COLOUR_MAGENTA  "\x1b[45m"
#define ANSI_BACKGROUND_COLOUR_CYAN     "\x1b[46m"
#define ANSI_BACKGROUND_COLOUR_WHITE    "\x1b[47m"
#define ANSI_COLOUR_RESET               "\x1b[0m"

#define COLOUR_BLACK                (0)
#define COLOUR_RED                  (1)
#define COLOUR_GREEN                (2)
#define COLOUR_YELLOW               (3)
#define COLOUR_BLUE                 (4)
#define COLOUR_MAGENTA              (5)
#define COLOUR_CYAN                 (6)
#define COLOUR_WHITE                (7)
#define BACKGROUND_COLOUR_BLACK     (128)
#define BACKGROUND_COLOUR_RED       (129)
#define BACKGROUND_COLOUR_GREEN     (130)
#define BACKGROUND_COLOUR_YELLOW    (131)
#define BACKGROUND_COLOUR_BLUE      (132)
#define BACKGROUND_COLOUR_MAGENTA   (133)
#define BACKGROUND_COLOUR_CYAN      (134)
#define BACKGROUND_COLOUR_WHITE     (135)

#endif  /* PG_BBASIC_INTERNAL_COLOURS_H */
