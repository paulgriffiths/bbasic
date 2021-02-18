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

/* Functions and types for built-in function expressions */

#include "internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <math.h>
#include <limits.h>

#if HAVE_FENV_H
#include <fenv.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "expr_internal.h"
#include "value.h"
#include "runtime.h"
#include "symbols.h"
#include "rand.h"
#include "terminal.h"
#include "util.h"

#define PI (3.14159265359)
#define BUFFER_SIZE (256)

/* Static function declarations */
static struct expr * expr_func_unary_new(struct expr * e,
        enum expr_type t);
static struct expr * expr_func_binary_new(struct expr * e,
        struct expr * f, enum expr_type t);
static struct expr * expr_func_ternary_new(struct expr * e,
        struct expr * f, struct expr * g, enum expr_type t);

static struct value * expr_eval_func_abs(struct expr * e);
static struct value * expr_eval_func_acs(struct expr * e);
static struct value * expr_eval_func_asc(struct expr * e);
static struct value * expr_eval_func_asn(struct expr * e);
static struct value * expr_eval_func_atn(struct expr * e);
static struct value * expr_eval_func_bget(struct expr * e);
static struct value * expr_eval_func_chrs(struct expr * e);
static struct value * expr_eval_func_cos(struct expr * e);
static struct value * expr_eval_func_deg(struct expr * e);
static struct value * expr_eval_func_eof(struct expr * e);
static struct value * expr_eval_func_erl(struct expr * e);
static struct value * expr_eval_func_err(struct expr * e);
static struct value * expr_eval_func_exp(struct expr * e);
static struct value * expr_eval_func_ext(struct expr * e);
static struct value * expr_eval_func_get(struct expr * e);
static struct value * expr_eval_func_gets(struct expr * e);
static struct value * expr_eval_func_inkey(struct expr * e);
static struct value * expr_eval_func_inkeys(struct expr * e);
static struct value * expr_eval_func_instr(struct expr * e);
static struct value * expr_eval_func_int(struct expr * e);
static struct value * expr_eval_func_lefts(struct expr * e);
static struct value * expr_eval_func_len(struct expr * e);
static struct value * expr_eval_func_ln(struct expr * e);
static struct value * expr_eval_func_log(struct expr * e);
static struct value * expr_eval_func_mids(struct expr * e);
static struct value * expr_eval_func_openin(struct expr * e);
static struct value * expr_eval_func_openout(struct expr * e);
static struct value * expr_eval_func_openup(struct expr * e);
static struct value * expr_eval_func_ptr(struct expr * e);
static struct value * expr_eval_func_rad(struct expr * e);
static struct value * expr_eval_func_rights(struct expr * e);
static struct value * expr_eval_func_rnd(struct expr * e);
static struct value * expr_eval_func_sgn(struct expr * e);
static struct value * expr_eval_func_sin(struct expr * e);
static struct value * expr_eval_func_spc(struct expr * e);
static struct value * expr_eval_func_sqr(struct expr * e);
static struct value * expr_eval_func_strings(struct expr * e);
static struct value * expr_eval_func_strs(struct expr * e);
static struct value * expr_eval_func_tan(struct expr * e);
static struct value * expr_eval_func_val(struct expr * e);


/*********************************************************************
 *                                                                   *
 * Constructor functions                                             *
 *                                                                   *
 *********************************************************************/

/* Creates a new ABS built-in function */
struct expr *
expr_func_abs_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_ABS);
    expr->eval = expr_eval_func_abs;
    return expr;
}

/* Creates a new ACS built-in function */
struct expr *
expr_func_acs_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_ACS);
    expr->eval = expr_eval_func_acs;
    return expr;
}

/* Creates a new ASN built-in function */
struct expr *
expr_func_asn_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_ASN);
    expr->eval = expr_eval_func_asn;
    return expr;
}

/* Creates a new ASC built-in function */
struct expr *
expr_func_asc_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_ASC);
    expr->eval = expr_eval_func_asc;
    return expr;
}

/* Creates a new ATN built-in function */
struct expr *
expr_func_atn_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_ATN);
    expr->eval = expr_eval_func_atn;
    return expr;
}

/* Creates a new BGET# built-in function */
struct expr *
expr_func_bget_new(struct expr * c) {
    struct expr * expr = expr_func_unary_new(c, EXPR_FUNC_BGET);
    expr->eval = expr_eval_func_bget;
    return expr;
}

/* Creates a new CHR$ built-in function */
struct expr *
expr_func_chrs_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_CHRS);
    expr->eval = expr_eval_func_chrs;
    return expr;
}

/* Creates a new COS built-in function */
struct expr *
expr_func_cos_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_COS);
    expr->eval = expr_eval_func_cos;
    return expr;
}

/* Creates a new DEG built-in function */
struct expr *
expr_func_deg_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_DEG);
    expr->eval = expr_eval_func_deg;
    return expr;
}

/* Creates a new EOF# built-in function */
struct expr *
expr_func_eof_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_EOF);
    expr->eval = expr_eval_func_eof;
    return expr;
}

/* Creates a new ERL built-in function */
struct expr *
expr_func_erl_new(void) {
    struct expr * expr = expr_new(EXPR_FUNC_ERL);
    expr->eval = expr_eval_func_erl;
    return expr;
}

/* Creates a new ERR built-in function */
struct expr *
expr_func_err_new(void) {
    struct expr * expr = expr_new(EXPR_FUNC_ERR);
    expr->eval = expr_eval_func_err;
    return expr;
}

/* Creates a new EXP built-in function */
struct expr *
expr_func_exp_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_EXP);
    expr->eval = expr_eval_func_exp;
    return expr;
}

/* Creates a new EXT# built-in function */
struct expr *
expr_func_ext_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_EXT);
    expr->eval = expr_eval_func_ext;
    return expr;
}

/* Creates a new GET built-in function */
struct expr *
expr_func_get_new(void) {
    struct expr * expr = expr_new(EXPR_FUNC_GET);
    expr->eval = expr_eval_func_get;
    return expr;
}

/* Creates a new GET$ built-in function */
struct expr *
expr_func_gets_new(void) {
    struct expr * expr = expr_new(EXPR_FUNC_GETS);
    expr->eval = expr_eval_func_gets;
    return expr;
}

/* Creates a new INKEY built-in function */
struct expr *
expr_func_inkey_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_INKEY);
    expr->eval = expr_eval_func_inkey;
    return expr;
}

/* Creates a new INKEY$ built-in function */
struct expr *
expr_func_inkeys_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_INKEYS);
    expr->eval = expr_eval_func_inkeys;
    return expr;
}

/* Creates a new INSTR built-in function */
struct expr *
expr_func_instr_new(struct expr * haystack,
        struct expr * needle, struct expr * start) {
    if ( start == NULL ) {
        start = expr_int_new(1);
    }

    struct expr * expr = expr_func_ternary_new(
            haystack, needle, start, EXPR_FUNC_INSTR);
    expr->eval = expr_eval_func_instr;
    return expr;
}

/* Creates a new INT built-in function */
struct expr *
expr_func_int_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_INT);
    expr->eval = expr_eval_func_int;
    return expr;
}

/* Creates a new LEFT$ built-in functions */
struct expr *
expr_func_lefts_new(struct expr * s, struct expr *n) {
    struct expr * expr = expr_func_binary_new(s, n, EXPR_FUNC_LEFTS);
    expr->eval = expr_eval_func_lefts;
    return expr;
}

/* Creates a new LEN built-in function */
struct expr *
expr_func_len_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_LEN);
    expr->eval = expr_eval_func_len;
    return expr;
}

/* Creates a new LN built-in function */
struct expr *
expr_func_ln_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_LN);
    expr->eval = expr_eval_func_ln;
    return expr;
}

/* Creates a new LOG built-in function */
struct expr *
expr_func_log_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_LOG);
    expr->eval = expr_eval_func_log;
    return expr;
}

/* Creates a new MID$ built-in function */
struct expr *
expr_func_mids_new(struct expr * s,
        struct expr * start, struct expr * len) {
    if ( len == NULL ) {
        len = expr_int_new(INT_MAX);
    }

    struct expr * expr = expr_func_ternary_new(s, start, len, EXPR_FUNC_MIDS);
    expr->eval = expr_eval_func_mids;
    return expr;
}

/* Creates a new OPENIN built-in function */
struct expr *
expr_func_openin_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_OPENIN);
    expr->eval = expr_eval_func_openin;
    return expr;
}

/* Creates a new OPENOUT built-in function */
struct expr *
expr_func_openout_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_OPENOUT);
    expr->eval = expr_eval_func_openout;
    return expr;
}

/* Creates a new OPENUP built-in function */
struct expr *
expr_func_openup_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_OPENUP);
    expr->eval = expr_eval_func_openup;
    return expr;
}

/* Creates a new PTR# built-in function */
struct expr *
expr_func_ptr_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_PTR);
    expr->eval = expr_eval_func_ptr;
    return expr;
}

/* Creates a new RAD built-in function */
struct expr *
expr_func_rad_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_RAD);
    expr->eval = expr_eval_func_rad;
    return expr;
}

/* Creates a new RIGHT$ built-in functions */
struct expr *
expr_func_rights_new(struct expr * s, struct expr *n) {
    struct expr * expr = expr_func_binary_new(s, n, EXPR_FUNC_RIGHTS);
    expr->eval = expr_eval_func_rights;
    return expr;
}

/* Creates a new RND built-in function */
struct expr *
expr_func_rnd_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_RND);
    expr->eval = expr_eval_func_rnd;
    return expr;
}

/* Creates a new SGN built-in function */
struct expr *
expr_func_sgn_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_SGN);
    expr->eval = expr_eval_func_sgn;
    return expr;
}

/* Creates a new SIN built-in function */
struct expr *
expr_func_sin_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_SIN);
    expr->eval = expr_eval_func_sin;
    return expr;
}

/* Creates a new SPC built-in function */
struct expr *
expr_func_spc_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_SPC);
    expr->eval = expr_eval_func_spc;
    return expr;
}

/* Creates a new SQR built-in function */
struct expr *
expr_func_sqr_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_SQR);
    expr->eval = expr_eval_func_sqr;
    return expr;
}

/* Creates a new STRING$ built-in function */
struct expr *
expr_func_strings_new(struct expr * n, struct expr * s) {
    struct expr * expr = expr_func_binary_new(s, n, EXPR_FUNC_STRINGS);
    expr->eval = expr_eval_func_strings;
    return expr;
}

/* Creates a new STR$ built-in function */
struct expr *
expr_func_strs_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_STRS);
    expr->eval = expr_eval_func_strs;
    return expr;
}

/* Creates a new TAN built-in function */
struct expr *
expr_func_tan_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_TAN);
    expr->eval = expr_eval_func_tan;
    return expr;
}

/* Creates a new VAL built-in function */
struct expr *
expr_func_val_new(struct expr * e) {
    struct expr * expr = expr_func_unary_new(e, EXPR_FUNC_VAL);
    expr->eval = expr_eval_func_val;
    return expr;
}


/*********************************************************************
 *                                                                   *
 * Type-checking functions                                           *
 *                                                                   *
 *********************************************************************/

/* Returns true if an expression is a built-in function */
bool
expr_is_builtin(struct expr * e) {
    switch ( e->type ) {
        case EXPR_FUNC_ABS:
        case EXPR_FUNC_ACS:
        case EXPR_FUNC_ASC:
        case EXPR_FUNC_ASN:
        case EXPR_FUNC_ATN:
        case EXPR_FUNC_BGET:
        case EXPR_FUNC_CHRS:
        case EXPR_FUNC_COS:
        case EXPR_FUNC_DEG:
        case EXPR_FUNC_EOF:
        case EXPR_FUNC_ERL:
        case EXPR_FUNC_ERR:
        case EXPR_FUNC_EXP:
        case EXPR_FUNC_EXT:
        case EXPR_FUNC_GET:
        case EXPR_FUNC_GETS:
        case EXPR_FUNC_INKEY:
        case EXPR_FUNC_INKEYS:
        case EXPR_FUNC_INSTR:
        case EXPR_FUNC_INT:
        case EXPR_FUNC_LEFTS:
        case EXPR_FUNC_LEN:
        case EXPR_FUNC_LN:
        case EXPR_FUNC_LOG:
        case EXPR_FUNC_MIDS:
        case EXPR_FUNC_OPENIN:
        case EXPR_FUNC_OPENOUT:
        case EXPR_FUNC_OPENUP:
        case EXPR_FUNC_PTR:
        case EXPR_FUNC_RAD:
        case EXPR_FUNC_RIGHTS:
        case EXPR_FUNC_RND:
        case EXPR_FUNC_SGN:
        case EXPR_FUNC_SIN:
        case EXPR_FUNC_SPC:
        case EXPR_FUNC_SQR:
        case EXPR_FUNC_STRINGS:
        case EXPR_FUNC_STRS:
        case EXPR_FUNC_TAN:
        case EXPR_FUNC_VAL:
            return true;

        default:
            break;
    }

    return false;
}


/*********************************************************************
 *                                                                   *
 * Sub-expression functions                                          *
 *                                                                   *
 *********************************************************************/

struct expr *
expr_sub(struct expr * e, const int n) {
    return e->subs[n];
}


/*********************************************************************
 *                                                                   *
 * Static common sub-constructor functions                           *
 *                                                                   *
 *********************************************************************/

/* Common sub-constructor for a unary function */
static struct expr *
expr_func_unary_new(struct expr * e, enum expr_type t) {
    struct expr * expr = expr_new(t);
    expr->subs[0] = e;
    return expr;
}

/* Common sub-constructor for a binary function */
static struct expr *
expr_func_binary_new(struct expr * e, struct expr * f, enum expr_type t) {
    struct expr * expr = expr_new(t);
    expr->subs[0] = e;
    expr->subs[1] = f;
    return expr;
}

/* Common sub-constructor for a ternary function */
static struct expr *
expr_func_ternary_new(struct expr * e, struct expr * f,
        struct expr * g, enum expr_type t) {
    struct expr * expr = expr_new(t);
    expr->subs[0] = e;
    expr->subs[1] = f;
    expr->subs[2] = g;
    return expr;
}


/*********************************************************************
 *                                                                   *
 * Static evaluation functions                                       *
 *                                                                   *
 *********************************************************************/

/* Evaluates an ABS built-in function */
static struct value *
expr_eval_func_abs(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    struct value * result = NULL;

    if ( value_is_float(v) ) {
        result = value_float_new(fabs(value_float(v)));
    } else if ( value_is_int(v) ) {
        result = value_int_new(abs(value_int(v)));
    } else {
        error_set(ERR_TYPE_MISMATCH);
    }

    value_free(v);

    return result;
}

/* Evaluates an ACS built-in function */
static struct value *
expr_eval_func_acs(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    if ( !value_is_numeric(v) ) {
        value_free(v);
        error_set(ERR_TYPE_MISMATCH);
        return NULL;
    }

    errno = 0;
    const double d = acos(value_float(v));
    if ( errno == EDOM ) {
        value_free(v);
        error_set(ERR_NEGATIVE_ROOT);
        return NULL;
    }

    value_free(v);

    return value_float_new(d);
}

/* Evaluates an ASC built-in function */
static struct value *
expr_eval_func_asc(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    if ( !value_is_string(v) ) {
        value_free(v);
        error_set(ERR_TYPE_MISMATCH);
        return NULL;
    }

    const char * s = value_string_peek(v);
    struct value * result = value_int_new(s[0] ? s[0] : -1);
    value_free(v);

    return result;
}

/* Evaluates an ASN built-in function */
static struct value *
expr_eval_func_asn(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    if ( !value_is_numeric(v) ) {
        value_free(v);
        error_set(ERR_TYPE_MISMATCH);
        return NULL;
    }

    errno = 0;
    const double d = asin(value_float(v));
    if ( errno == EDOM ) {
        value_free(v);
        error_set(ERR_NEGATIVE_ROOT);
        return NULL;
    }

    value_free(v);

    return value_float_new(d);
}

/* Evaluates an ATN built-in function */
static struct value *
expr_eval_func_atn(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    if ( !value_is_numeric(v) ) {
        value_free(v);
        error_set(ERR_TYPE_MISMATCH);
        return NULL;
    }

    struct value * result = value_float_new(atan(value_float(v)));
    value_free(v);

    return result;
}

/* Evaluates a BGET# built-in function */
static struct value *
expr_eval_func_bget(struct expr * e) {
    struct value * c = expr_eval(e->subs[0]);
    if ( !c ) {
        return NULL;
    } else if ( !value_is_int(c) ) {
        value_free(c);
        error_set(ERR_TYPE_MISMATCH);
        return NULL;
    }

    const int fd = value_int(c);
    value_free(c);

    /* Disallow operations on stdin, stdout and stderr */
    if ( fd < 3 ) {
        error_set(ERR_CHANNEL);
        return NULL;
    }

    unsigned char in;
    int status = read(fd, &in, 1);
    if ( status == -1 ) {
        error_set(ERR_CHANNEL);
        return NULL;
    } else if ( status == 0 ) {
        error_set(ERR_EOF);
        return NULL;
    }

    return value_int_new(in);
}

/* Evaluates a CHR$ built-in function */
static struct value *
expr_eval_func_chrs(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    if ( !value_is_numeric(v) ) {
        value_free(v);
        error_set(ERR_TYPE_MISMATCH);
        return NULL;
    }

    struct value * result = value_string_new((char[]){value_int(v), 0});
    value_free(v);

    return result;
}

/* Evaluates a COS built-in function */
static struct value *
expr_eval_func_cos(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    if ( !value_is_numeric(v) ) {
        value_free(v);
        error_set(ERR_TYPE_MISMATCH);
        return NULL;
    }

    errno = 0;
    const double d = cos(value_float(v));
    if ( errno == EDOM ) {
        value_free(v);
        error_set(ERR_NEGATIVE_ROOT);
        return NULL;
    }

    value_free(v);

    return value_float_new(d);
}

/* Evaluates a DEG built-in function */
static struct value *
expr_eval_func_deg(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    if ( !value_is_numeric(v) ) {
        value_free(v);
        error_set(ERR_TYPE_MISMATCH);
        return NULL;
    }

    struct value * result = value_float_new((value_float(v)) * 180 / PI);
    value_free(v);

    return result;
}

/* Evaluates an EOF# built-in function.
 * NOTE: The EOF# is a bug on a multi-user system, where end-of-file
 * is always the result of a failed read, since even if the file
 * offset is at the end of the file at the time of the EOF check,
 * it's always possible at another process might add more data to
 * the file, and the next read could succeed in reading more data.
 * However, BBC BASIC II provides it, so we implement it, and users
 * can only rely on this function if they're sure no other process
 * will be writing to the file.
 */
static struct value *
expr_eval_func_eof(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    } else if ( !value_is_int(v) ) {
        value_free(v);
        error_set(ERR_TYPE_MISMATCH);
        return NULL;
    }

    const int fd = value_int(v);
    value_free(v);

    /* Disallow operations on stdin, stdout and stderr */
    if ( fd < 3 ) {
        error_set(ERR_CHANNEL);
        return NULL;
    }

    /* Get file stats so we can get the file size */
    struct stat statbuf;
    if ( fstat(fd, &statbuf) == -1 ) {
        error_set(ERR_CHANNEL);
        return NULL;
    }

    /* Check we're dealing with a regular file */
    if ( (statbuf.st_mode & S_IFMT) != S_IFREG ) {
        error_set(ERR_CHANNEL);
        return NULL;
    }

    /* Get the current file offset */
    const off_t pos = lseek(fd, 0, SEEK_CUR);
    if ( pos == -1 ) {
        error_set(ERR_CHANNEL);
        return NULL;
    }

    /* Compare offset to size and return appropriate boolean value */
    return value_int_new(pos == statbuf.st_size ? -1 : 0);
}

/* Evaluates an ERL built-in function */
static struct value *
expr_eval_func_erl(struct expr * e) {
    return value_int_new(error_last_line());
}

/* Evaluates a ERR built-in function */
static struct value *
expr_eval_func_err(struct expr * e) {
    int code = error_last_code();
    if ( code == ERR_NO_ERROR ) {
        /* Return 0 instead of the internal "no error" code */
        code = 0;
    }

    return value_int_new(code);
}

/* Evaluates an EXP built-in function */
static struct value *
expr_eval_func_exp(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    if ( !value_is_numeric(v) ) {
        value_free(v);
        error_set(ERR_TYPE_MISMATCH);
        return NULL;
    }

    errno = 0;
    const double d = exp(value_float(v));
    if ( errno == ERANGE ) {
        value_free(v);
        error_set(ERR_NEGATIVE_ROOT);
        return NULL;
    }

    value_free(v);

    return value_float_new(d);
}

/* Evaluates an EXT# built-in function */
static struct value *
expr_eval_func_ext(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    } else if ( !value_is_int(v) ) {
        value_free(v);
        error_set(ERR_TYPE_MISMATCH);
        return NULL;
    }

    const int fd = value_int(v);
    value_free(v);

    /* Disallow operations on stdin, stdout and stderr */
    if ( fd < 3 ) {
        error_set(ERR_CHANNEL);
        return NULL;
    }

    struct stat statbuf;
    if ( fstat(fd, &statbuf) == -1 ) {
        error_set(ERR_CHANNEL);
        return NULL;
    }

    /* Check we're dealing with a regular file */
    if ( (statbuf.st_mode & S_IFMT) != S_IFREG ) {
        error_set(ERR_CHANNEL);
        return NULL;
    }

    if ( statbuf.st_size > INT32_MAX ) {
        ABORT("file too big");
    }

    return value_int_new(statbuf.st_size);
}

/* Evaluates an GET built-in function */
static struct value *
expr_eval_func_get(struct expr * e) {
    int c = get_char(-1);
    if ( c == EOF ) {
        return NULL;
    } else if ( c == 0 ) {
        return value_int_new(-1);
    }

    return value_int_new(c);
}

/* Evaluates an GET$ built-in function */
static struct value *
expr_eval_func_gets(struct expr * e) {
    int c = get_char(-1);
    if ( c == EOF ) {
        return NULL;
    } else if ( c == 0 ) {
        return value_string_new("");
    }

    return value_string_new((char[]){c, 0});
}

/* Evaluates an INKEY built-in function */
static struct value *
expr_eval_func_inkey(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    } else if ( !value_is_numeric(v) ) {
        error_set(ERR_TYPE_MISMATCH);
        value_free(v);
        return NULL;
    }

    const int hsecs = value_int(v);
    value_free(v);

    int c = get_char(hsecs);
    if ( c == EOF ) {
        return NULL;
    } else if ( c == 0 ) {
        return value_int_new(-1);
    }

    return value_int_new(c);
}

/* Evaluates an INKEY$ built-in function */
static struct value *
expr_eval_func_inkeys(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    } else if ( !value_is_numeric(v) ) {
        error_set(ERR_TYPE_MISMATCH);
        value_free(v);
        return NULL;
    }

    const int hsecs = value_int(v);
    value_free(v);

    int c = get_char(hsecs);
    if ( c == EOF ) {
        return NULL;
    } else if ( c == 0 ) {
        return value_string_new("");
    }

    return value_string_new((char[]){c, 0});
}

/* Evaluates a INSTR built-in function */
static struct value *
expr_eval_func_instr(struct expr * e) {
    struct value * hval = expr_eval(e->subs[0]);
    if ( !hval ) {
        return NULL;
    }

    struct value * nval = expr_eval(e->subs[1]);
    if ( !nval ) {
        value_free(hval);
        return NULL;
    }

    struct value * sval = expr_eval(e->subs[2]);
    if ( !sval ) {
        value_free(hval);
        value_free(nval);
        return NULL;
    }

    if ( !value_is_string(hval) || !value_is_string(nval)
            || !value_is_numeric(sval) ) {
        error_set(ERR_TYPE_MISMATCH);
        value_free(hval);
        value_free(nval);
        value_free(sval);
        return NULL;
    }

    size_t start = value_int(sval);
    start = start ? start - 1 : 0;
    const char * h = value_string_peek(hval);
    const char * n = value_string_peek(nval);

    value_free(sval);

    struct value * result = NULL;

    if ( strlen(n) == 0 ) {
        /* Always return 1 if the needle is the empty string */
        result = value_int_new(1);
    } else if ( start >= strlen(h) ) {
        /* Start index is past the end of the string */
        result = value_int_new(0);
    } else {
        /* Defer the rest to strstr */
        char * s = strstr(h + start, n);
        result = value_int_new(s ? s - h + 1 : 0);
    }

    value_free(hval);
    value_free(nval);

    return result;
}

/* Evaluates an INT built-in function */
static struct value *
expr_eval_func_int(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    if ( !value_is_numeric(v) ) {
        value_free(v);
        error_set(ERR_TYPE_MISMATCH);
        return NULL;
    }

    struct value * result = value_int_new(floor(value_float(v)));
    value_free(v);

    return result;
}

/* Evaluates a LEFT$ built-in function */
static struct value *
expr_eval_func_lefts(struct expr * e) {
    struct value * sval = expr_eval(e->subs[0]);
    if ( !sval ) {
        return NULL;
    }

    struct value * nval = expr_eval(e->subs[1]);
    if ( !nval ) {
        value_free(sval);
        return NULL;
    }

    if ( !value_is_string(sval) || !value_is_numeric(nval) ) {
        error_set(ERR_TYPE_MISMATCH);
        value_free(sval);
        value_free(nval);
        return NULL;
    }

    char * s = value_string(sval);
    const size_t n = value_int(nval);

    value_free(sval);
    value_free(nval);

    if ( n < strlen(s) ) {
        /* Shorten the string if necessary */
        s[n] = '\0';
    }

    struct value * result = value_string_new(s);
    free(s);

    return result;
}

/* Evaluates a LEN built-in function */
static struct value *
expr_eval_func_len(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    if ( !value_is_string(v) ) {
        value_free(v);
        error_set(ERR_TYPE_MISMATCH);
        return NULL;
    }

    struct value * result = value_int_new(strlen(value_string_peek(v)));
    value_free(v);

    return result;
}

/* Evaluates an LN built-in function */
static struct value *
expr_eval_func_ln(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    if ( !value_is_numeric(v) ) {
        value_free(v);
        error_set(ERR_TYPE_MISMATCH);
        return NULL;
    }

    errno = 0;
    const double d = log(value_float(v));
    if ( (errno == EDOM) || (errno == ERANGE) ) {
        value_free(v);
        error_set(ERR_LOG_RANGE);
        return NULL;
    }

    value_free(v);

    return value_float_new(d);
}

/* Evaluates a LOG built-in function */
static struct value *
expr_eval_func_log(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    if ( !value_is_numeric(v) ) {
        value_free(v);
        error_set(ERR_TYPE_MISMATCH);
        return NULL;
    }

    errno = 0;
    const double d = log10(value_float(v));
    if ( (errno == EDOM) || (errno == ERANGE) ) {
        value_free(v);
        error_set(ERR_LOG_RANGE);
        return NULL;
    }

    value_free(v);

    return value_float_new(d);
}

/* Evaluates a MID$ built-in function */
static struct value *
expr_eval_func_mids(struct expr * e) {
    struct value * sval = expr_eval(e->subs[0]);
    if ( !sval ) {
        return NULL;
    }

    struct value * stval = expr_eval(e->subs[1]);
    if ( !stval ) {
        value_free(sval);
        return NULL;
    }

    struct value * lval = expr_eval(e->subs[2]);
    if ( !lval ) {
        value_free(sval);
        value_free(stval);
        return NULL;
    }

    if ( !value_is_string(sval) || !value_is_numeric(stval)
            || !value_is_numeric(lval) ) {
        error_set(ERR_TYPE_MISMATCH);
        value_free(sval);
        value_free(stval);
        value_free(lval);
        return NULL;
    }

    char * s = value_string_peek(sval);
    const size_t l = strlen(s);
    size_t start = value_int(stval);
    const size_t len = value_int(lval);

    if ( start > l) {
        /* Start is past end of string, so set it to the
         * index of the terminating null
         */
        start = l;
    } else if ( start != 0 ) {
        /* MID$ uses 1-based indexing, so subtract one, but
         * just accept 0 if it was provided.
         */
        start -= 1;
    }

    s = x_strdup(s + start);
    if ( (start + len) < l ) {
        /* Shorten the string if necessary */
        s[len] = '\0';
    }

    struct value * result = value_string_new(s);

    free(s);
    value_free(sval);
    value_free(stval);
    value_free(lval);

    return result;
}

/* Evaluates an OPENIN built-in function */
static struct value *
expr_eval_func_openin(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    } else if ( !value_is_string(v) ) {
        error_set(ERR_TYPE_MISMATCH);
        value_free(v);
        return NULL;
    }

    struct value * result = NULL;

    int fd = open(value_string_peek(v), O_RDONLY);
    if ( fd == -1 ) {
        result = value_int_new(0);
    } else {
        result = value_int_new(fd);
        open_file_add(fd);
    }

    value_free(v);

    return result;
}

/* Evaluates an OPENOUT built-in function */
static struct value *
expr_eval_func_openout(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    } else if ( !value_is_string(v) ) {
        error_set(ERR_TYPE_MISMATCH);
        value_free(v);
        return NULL;
    }

    struct value * result = NULL;

    int fd = open(value_string_peek(v), O_CREAT | O_TRUNC | O_WRONLY,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if ( fd == -1 ) {
        result = value_int_new(0);
    } else {
        result = value_int_new(fd);
        open_file_add(fd);
    }

    value_free(v);

    return result;
}

/* Evaluates an OPENUP built-in function */
static struct value *
expr_eval_func_openup(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    } else if ( !value_is_string(v) ) {
        error_set(ERR_TYPE_MISMATCH);
        value_free(v);
        return NULL;
    }

    struct value * result = NULL;

    int fd = open(value_string_peek(v), O_RDWR);
    if ( fd == -1 ) {
        result = value_int_new(0);
    } else {
        result = value_int_new(fd);
        open_file_add(fd);
    }

    value_free(v);

    return result;
}

/* Evaluates a PTR# built-in function */
static struct value *
expr_eval_func_ptr(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    if ( !value_is_numeric(v) ) {
        value_free(v);
        error_set(ERR_TYPE_MISMATCH);
        return NULL;
    }

    const int fd = value_int(v);
    value_free(v);

    /* Disallow operations on stdin, stdout and stderr */
    if ( fd < 3 ) {
        error_set(ERR_CHANNEL);
        return NULL;
    }

    const int ptr = open_file_get_ptr(fd);
    if ( ptr == STATUS_ERROR ) {
        return NULL;
    }

    return value_int_new(ptr);
}

/* Evaluates a RAD built-in function */
static struct value *
expr_eval_func_rad(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    if ( !value_is_numeric(v) ) {
        value_free(v);
        error_set(ERR_TYPE_MISMATCH);
        return NULL;
    }

    struct value * result = value_float_new((value_float(v)) * PI / 180);
    value_free(v);

    return result;
}

/* Evaluates a RIGHT$ built-in function */
static struct value *
expr_eval_func_rights(struct expr * e) {
    struct value * sval = expr_eval(e->subs[0]);
    if ( !sval ) {
        return NULL;
    }

    struct value * nval = expr_eval(e->subs[1]);
    if ( !nval ) {
        value_free(sval);
        return NULL;
    }

    if ( !value_is_string(sval) || !value_is_numeric(nval) ) {
        error_set(ERR_TYPE_MISMATCH);
        value_free(sval);
        value_free(nval);
        return NULL;
    }

    char * s = value_string_peek(sval);
    const size_t n = value_int(nval);
    const size_t l = strlen(s);
    if ( n < l ) {
        /* Form a right substing */
        s = x_strdup(s+l-n);
    } else {
        /* n is big enough to encompass the entire string */
        s = x_strdup(s);
    }

    struct value * result = value_string_new(s);

    free(s);
    value_free(sval);
    value_free(nval);

    return result;
}

/* Evaluates a RND built-in function
 * NOTE: the quality of random numbers generated by this function
 * is poor, and it's possible to do a lot better. However, the PRNG
 * in BBC BASIC II is at least as poor, so we won't worry about it.
 */
static struct value *
expr_eval_func_rnd(struct expr * e) {
    static double last_rnd_1 = 0.255162656;

    if ( !e->subs[0] ) {
        /* Without any arguments, RND is documented to return a
         * random whole number between -2147483648 and 2147483647.
         * Rather than spend time messing about with potentially
         * varying values of RAND_MAX (which may be far less than
         * that range) we'll just generate four random bytes and
         * poke them into a 32 bit integer.
         */

        int32_t n;
        for ( size_t i = 0; i < sizeof(n); i++ ) {
            ((unsigned char *)&n)[i] = get_random(256) - 1;
        }

        return value_int_new(n);
    }

    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    if ( !value_is_numeric(v) ) {
        error_set(ERR_TYPE_MISMATCH);
        value_free(v);
        return NULL;
    }

    struct value * result = NULL;

    const int n = value_int(v);
    if ( n == 0 ) {
        /* RND(0) returns the last number given by RND(1) */
        result = value_float_new(last_rnd_1);
    } else if ( n == 1 ) {
        /* RND(1) returns a random float between 0 and 0.999999 */
        last_rnd_1 = rand() / (RAND_MAX + 1.0);
        result = value_float_new(last_rnd_1);
    } else if ( n < 0 ) {
        /* RND(-X) returns -X and seeds the random number generator
         * to a value based on X. Note that while BBC BASIC II is
         * completely deterministic in this regard, this implementation
         * is not, and repeated calls to RND(-X) with the same X
         * will seed the random number generator with different values,
         * and the value of X really doesn't matter. We do not consider
         * this a defect.
         */
        seed_prng(-n);
        result = value_int_new(n);
    } else {
        /* RND(X) generates a random number between 1 and X, inclusive */
        result = value_int_new(get_random(n));
    }

    value_free(v);

    return result;
}

/* Evaluates a SGN built-in function */
static struct value *
expr_eval_func_sgn(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    if ( !value_is_numeric(v) ) {
        value_free(v);
        error_set(ERR_TYPE_MISMATCH);
        return NULL;
    }

    const double d = value_float(v);
    struct value * result = value_int_new(d == 0 ? 0 : (d < 0 ? -1 : 1));

    value_free(v);

    return result;
}

/* Evaluates a SIN built-in function */
static struct value *
expr_eval_func_sin(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    if ( !value_is_numeric(v) ) {
        value_free(v);
        error_set(ERR_TYPE_MISMATCH);
        return NULL;
    }

    errno = 0;
    const double d = sin(value_float(v));
    if ( errno == EDOM ) {
        value_free(v);
        error_set(ERR_NEGATIVE_ROOT);
        return NULL;
    }

    value_free(v);

    return value_float_new(d);
}

/* Evaluates an SPC built-in function */
static struct value *
expr_eval_func_spc(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    if ( !value_is_numeric(v) ) {
        value_free(v);
        error_set(ERR_TYPE_MISMATCH);
        return NULL;
    }

    /* Up to 255 spaces may be printed */
    const int n = value_int(v) % 256;
    value_free(v);

    char s[BUFFER_SIZE] = {0};
    for ( size_t i = 0; i < n; i++ ) {
        s[i] = ' ';
    }

    return value_string_new(s);
}

/* Evaluates an SQR built-in function */
static struct value *
expr_eval_func_sqr(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    if ( !value_is_numeric(v) ) {
        value_free(v);
        error_set(ERR_TYPE_MISMATCH);
        return NULL;
    }

    errno = 0;
    const double d = sqrt(value_float(v));
    if ( errno == EDOM ) {
        value_free(v);
        error_set(ERR_NEGATIVE_ROOT);
        return NULL;
    }

    value_free(v);

    return value_float_new(d);
}

/* Evaluates a STRING$ built-in function */
static struct value *
expr_eval_func_strings(struct expr * e) {
    struct value * sval = expr_eval(e->subs[0]);
    if ( !sval ) {
        return NULL;
    }

    struct value * nval = expr_eval(e->subs[1]);
    if ( !nval ) {
        value_free(sval);
        return NULL;
    }

    if ( !value_is_string(sval) || !value_is_numeric(nval) ) {
        error_set(ERR_TYPE_MISMATCH);
        value_free(sval);
        value_free(nval);
        return NULL;
    }

    /* Limit n to 256 */
    const size_t n = value_int(nval) % 256;
    const char * s = value_string_peek(sval);
    const size_t l = strlen(s);

    /* calloc to get a terminating null even if l == 0 */
    char * cat = x_calloc(n * l + 1, 1);
    for ( size_t i = 0; i < n; i++ ) {
        strcpy(cat + i * l, s);
    }

    struct value * result = value_string_new(cat);

    free(cat);
    value_free(sval);
    value_free(nval);

    return result;
}

/* Evaluates a STR$ built-in function */
static struct value *
expr_eval_func_strs(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    if ( !value_is_numeric(v) ) {
        error_set(ERR_TYPE_MISMATCH);
        value_free(v);
        return NULL;
    }

    char format[BUFFER_SIZE] = {0};
    const int places = format_places();
    const int width = format_width();

    switch ( format_number() ) {
        case FORMAT_NORMAL:
            sprintf(format, "%%%d.%dG", width, places);
            break;

        case FORMAT_SCIENTIFIC:
            /* Note this probably won't match the format on
             * the BBC Micro, where printing 2.345 with
             * @%=&010209 would give 2.3E0, where 2.3E+00
             * is more normal here. We'll accept the difference
             * for now.
             */
            sprintf(format, "%%%d.%dE", width, places == 0 ? 0 : places - 1);
            break;

        case FORMAT_FIXED:
            sprintf(format, "%%%d.%dF", width, places);
            break;

        default:
            ABORTF("unexpected format: %d\n", format_number());
    }

    char buffer[BUFFER_SIZE]; /* Should be big enough to hold any number */
    const int n = snprintf(buffer, BUFFER_SIZE, format, value_float(v));
    if ( n >= BUFFER_SIZE ) {
        ABORTF("string too big (%d) for buffer (%d)", n, BUFFER_SIZE);
    }

    struct value * result = value_string_new(buffer);

    value_free(v);

    return result;
}

/* Evaluates a TAN built-in function */
static struct value *
expr_eval_func_tan(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    if ( !value_is_numeric(v) ) {
        value_free(v);
        error_set(ERR_TYPE_MISMATCH);
        return NULL;
    }

    int status;
    if ( (status = feclearexcept(FE_OVERFLOW)) != 0 ) {
        ABORTF("feclearexcept failed with status %d", status);
    }

    errno = 0;
    const double d = tan(value_float(v));
    if ( (errno == EDOM) || fetestexcept(FE_OVERFLOW) ) {
        error_set(ERR_NEGATIVE_ROOT);
        value_free(v);
        return NULL;
    }

    value_free(v);

    return value_float_new(d);
}

/* Evaluates a VAL built-in function */
static struct value *
expr_eval_func_val(struct expr * e) {
    struct value * v = expr_eval(e->subs[0]);
    if ( !v ) {
        return NULL;
    }

    if ( !value_is_string(v) ) {
        error_set(ERR_TYPE_MISMATCH);
        value_free(v);
        return NULL;
    }

    const char * s = value_string_peek(v);
    char * endptr;
    errno = 0;
    const double d = strtod(s,  &endptr);
    if ( errno == ERANGE ) {
        error_set(ERR_TOO_BIG);
        value_free(v);
        return NULL;
    }

    struct value * result = value_float_new(endptr == s ? 0 : d);
    value_free(v);

    return result;
}
