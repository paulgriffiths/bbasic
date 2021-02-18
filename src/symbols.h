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

#ifndef PG_BBASIC_INTERNAL_SYMBOLS_H
#define PG_BBASIC_INTERNAL_SYMBOLS_H

#include <stdbool.h>
#include "expr.h"
#include "value.h"
#include "runtime.h"

/* Opaque and incomplete struct definition */
struct statement;

/* Number formats in @% resident integer variable */
enum number_format {
    FORMAT_NORMAL = 0,
    FORMAT_SCIENTIFIC = 1,
    FORMAT_FIXED = 2
};

/* Symbol table functions */
void symbol_table_clear(void);
void symbol_table_free(void);
void symbol_table_init(void);
void symbol_table_pop_frame(void);
void symbol_table_push_frame(void);

/* Procedure functions */
enum basic_error pass_arguments(struct expr * params, struct expr * args);
struct statement * symbol_proc_call(const char * id);
int symbol_proc_define(const char * id, struct statement * stmt);

/* Variable functions */
int symbol_array_assign(const char * id, struct value * indices, struct value * v);
int symbol_array_dimension(const char * id, struct value * v);
struct value * symbol_array_eval(const char * id, struct value * indices);
int symbol_variable_assign(const char * id, struct value * v);
int symbol_variable_assign_local(const char * id, struct value * v);
struct value * symbol_variable_eval(const char * id);

/* Variable name functions */
bool variable_name_is_integer(const char * s);
bool variable_name_is_real(const char * s);
bool variable_name_is_resident(const char * s);
bool variable_name_is_string(const char * s);

/* Number format functions */
enum number_format format_number(void);
int format_places(void);
void format_reset(void);
int format_width(void);

#endif  /* PG_BBASIC_INTERNAL_SYMBOLS_H */
