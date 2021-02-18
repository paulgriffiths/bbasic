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

#ifndef PG_BBASIC_INTERNAL_EXPR_BUILTIN_H
#define PG_BBASIC_INTERNAL_EXPR_BUILTIN_H

#include <stdbool.h>

/* Opaque and incomplete struct definition */
struct expr;

/* Constructors */
struct expr * expr_func_abs_new(struct expr * e);
struct expr * expr_func_acs_new(struct expr * e);
struct expr * expr_func_asc_new(struct expr * e);
struct expr * expr_func_asn_new(struct expr * e);
struct expr * expr_func_atn_new(struct expr * e);
struct expr * expr_func_bget_new(struct expr * c);
struct expr * expr_func_chrs_new(struct expr * e);
struct expr * expr_func_cos_new(struct expr * e);
struct expr * expr_func_deg_new(struct expr * e);
struct expr * expr_func_eof_new(struct expr * e);
struct expr * expr_func_erl_new(void);
struct expr * expr_func_err_new(void);
struct expr * expr_func_exp_new(struct expr * e);
struct expr * expr_func_ext_new(struct expr * e);
struct expr * expr_func_get_new(void);
struct expr * expr_func_gets_new(void);
struct expr * expr_func_inkey_new(struct expr * e);
struct expr * expr_func_inkeys_new(struct expr * e);
struct expr * expr_func_instr_new(struct expr * haystack,
        struct expr * needle, struct expr * start);
struct expr * expr_func_int_new(struct expr * e);
struct expr * expr_func_lefts_new(struct expr * s, struct expr * n);
struct expr * expr_func_len_new(struct expr * e);
struct expr * expr_func_ln_new(struct expr * e);
struct expr * expr_func_log_new(struct expr * e);
struct expr * expr_func_mids_new(struct expr * s,
        struct expr * start, struct expr * len);
struct expr * expr_func_openin_new(struct expr * e);
struct expr * expr_func_openout_new(struct expr * e);
struct expr * expr_func_openup_new(struct expr * e);
struct expr * expr_func_ptr_new(struct expr * e);
struct expr * expr_func_rad_new(struct expr * e);
struct expr * expr_func_rights_new(struct expr * s, struct expr * n);
struct expr * expr_func_rnd_new(struct expr * e);
struct expr * expr_func_sgn_new(struct expr * e);
struct expr * expr_func_sin_new(struct expr * e);
struct expr * expr_func_spc_new(struct expr * e);
struct expr * expr_func_sqr_new(struct expr * e);
struct expr * expr_func_strings_new(struct expr * n, struct expr * s);
struct expr * expr_func_strs_new(struct expr * e);
struct expr * expr_func_tan_new(struct expr * e);
struct expr * expr_func_val_new(struct expr * e);

/* Type-checking function */
bool expr_is_builtin(struct expr * e);

/* Sub-expression function */
struct expr * expr_sub(struct expr * e, const int n);

#endif  /* PG_BBASIC_INTERNAL_EXPR_BUILTIN_H */
