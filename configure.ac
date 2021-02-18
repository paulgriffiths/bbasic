#  BBASIC, an interpreter for a subset of BBC BASIC II.
#  Copyright (C) 2021 Paul Griffiths.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3, or (at your option)
#  any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; If not, see <https://www.gnu.org/licenses/>.

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([BBasic], [0.9.0])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_RANLIB
AC_PROG_LEX
AC_PROG_YACC
AC_PROG_INSTALL
AC_CHECK_PROGS([FLEX], [flex])

if test "x${ac_cv_prog_cc_c99}" = "xno"; then
    AC_MSG_ERROR([C99 compiler required])
fi

# Colour option
AC_ARG_ENABLE([ansi-colours],
              [AS_HELP_STRING([--disable-ansi-colours],
                              [disable ANSI colours @<:@default: no@:>@])],
              [ansi_colours=${enableval}], [ansi_colours=yes])

if test "x${ansi_colours}" = xyes; then
    AC_DEFINE([ENABLE_ANSI_COLOURS], 1, [ANSI colours enabled])
fi

# Checks for libraries.
AC_CHECK_LIB([m], [acos])

# Checks for header files.
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([fcntl.h fenv.h inttypes.h libintl.h limits.h malloc.h stddef.h stdlib.h string.h unistd.h getopt.h sys/time.h termios.h sys/select.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT8_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([atexit clock_gettime memmove memset strdup strerror strtol strstr getopt getopt_long floor sqrt pow getpid select])

AX_COMPILER_FLAGS

AC_CONFIG_FILES([Makefile
                 pgcommon/Makefile
                 samples/Makefile
                 src/Makefile])
AC_OUTPUT
