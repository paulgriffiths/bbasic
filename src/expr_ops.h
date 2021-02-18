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

#ifndef PG_BBASIC_INTERNAL_EXPR_OPS_H
#define PG_BBASIC_INTERNAL_EXPR_OPS_H

/* Opaque and incomplete struct definition */
struct expr;

/* Constructors */
struct expr * expr_op_add_new(struct expr * l, struct expr * r);
struct expr * expr_op_and_new(struct expr * l, struct expr * r);
struct expr * expr_op_div_new(struct expr * l, struct expr * r);
struct expr * expr_op_eor_new(struct expr * l, struct expr * r);
struct expr * expr_op_eq_new(struct expr * l, struct expr * r);
struct expr * expr_op_exp_new(struct expr * l, struct expr * r);
struct expr * expr_op_gt_new(struct expr * l, struct expr * r);
struct expr * expr_op_gte_new(struct expr * l, struct expr * r);
struct expr * expr_op_idiv_new(struct expr * l, struct expr * r);
struct expr * expr_op_lt_new(struct expr * l, struct expr * r);
struct expr * expr_op_lte_new(struct expr * l, struct expr * r);
struct expr * expr_op_mod_new(struct expr * l, struct expr * r);
struct expr * expr_op_mul_new(struct expr * l, struct expr * r);
struct expr * expr_op_neq_new(struct expr * l, struct expr * r);
struct expr * expr_op_not_new(struct expr * e);
struct expr * expr_op_or_new(struct expr * l, struct expr * r);
struct expr * expr_op_sub_new(struct expr * l, struct expr * r);
struct expr * expr_op_uminus_new(struct expr * e);

#endif  /* PG_BBASIC_INTERNAL_EXPR_OPS_H */
