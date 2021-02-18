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

BUILT_SOURCES = parser.h
AM_YFLAGS = -d -v
bin_PROGRAMS = bbasic
bbasic_SOURCES = main.c lexer.l parser.y yydecls.h runtime.c runtime.h statements.c statements.h expr.c expr.h options.c options.h symbols.c symbols.h line_map.c line_map.h stack_addr.c stack_addr.h addr_set.c addr_set.h expr_internal.h expr_value.c expr_value.h expr_builtin.c expr_builtin.h expr_ops.c expr_ops.h rand.c rand.h value.c value.h expr_fn.c expr_fn.h colours.h data_map.c data_map.h terminal.h terminal.c file_set.c file_set.h
bbasic_CPPFLAGS = -I$(top_srcdir)/pgcommon
bbasic_LDADD = ../pgcommon/libpgcommon.a

EXTRA_DIST = arith_test.basic array_test.basic branch_test.basic error_test.basic files_test.basic strings_test.basic test_in.file

check_SCRIPTS = arith_test.sh array_test.sh branch_test.sh rt_errors_test.sh strings_test.sh
TESTS = $(check_SCRIPTS)

array_test.sh:
	echo 'set -e' > array_test.sh
	echo "./bbasic ${srcdir}/array_test.basic" >> array_test.sh
	chmod +x array_test.sh

arith_test.sh:
	echo 'set -e' > arith_test.sh
	echo "./bbasic ${srcdir}/arith_test.basic" >> arith_test.sh
	chmod +x arith_test.sh

branch_test.sh:
	echo 'set -e' > branch_test.sh
	echo "./bbasic ${srcdir}/branch_test.basic" >> branch_test.sh
	chmod +x branch_test.sh

rt_errors_test.sh:
	echo 'set -e' > rt_errors_test.sh
	echo "./bbasic ${srcdir}/error_test.basic" >> rt_errors_test.sh
	chmod +x rt_errors_test.sh

strings_test.sh:
	echo 'set -e' > strings_test.sh
	echo "./bbasic ${srcdir}/strings_test.basic" >> strings_test.sh
	chmod +x strings_test.sh

CLEANFILES = arith_test.sh array_test.sh branch_test.sh rt_errors_test.sh strings_test.sh test_out.file
