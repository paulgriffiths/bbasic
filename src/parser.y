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

/* bbasic parser */

%{
#include "internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

#include "parser.h"
#include "runtime.h"
#include "statements.h"
#include "expr_internal.h"

int yyerror(char * s, ...);
int yylex(void);
extern int yylineno;

%}

/* Token types */
%union {
    char * s;
    double d;
    int32_t n;
    struct expr * ex;
    struct statement * st;
    struct print_item * pi;
    struct value * v;
}

/* Most keywords */
%token ABS ACS AND ASC ASN ATN AUTO BGET BPUT CHAIN CHRS CLEAR
%token CLOSE COLOUR COS DATA DEF DEG DELETE DIM DIV ELSE END
%token ENDPROC EOF_ EOR ERL ERR ERROR EVAL EXP EXT FOR GET GETS
%token GOSUB GOTO IF INKEY INKEYS INPUT INPUT_ INSTR INT LEFTS
%token LEN LET LINE LIST LISTO LN LOAD LOCAL LOG MIDS MOD NEW
%token NEXT NOT OFF OLD ON OPENIN OPENOUT OPENUP OR PI PRINT
%token PRINT_ PTR RAD READ RENUMBER REPEAT REPORT RESTORE RETURN
%token RIGHTS RND RUN SAVE SGN SIN SPC SQR STEP STOP STRS
%token STRINGS TAN THEN TO TRACE UNTIL VAL

/* Keywords with values */
%token <n> FALSE TRUE
%token <s> FN PROC REM

/* Operators */
%token OP_EQ OP_GT OP_GTE OP_LT OP_LTE OP_NEQ

/* Literals and identifiers */
%token <d> FLOAT_LITERAL
%token <n> HEX_LITERAL INT_LITERAL
%token <s> STRING_LITERAL
%token <s> ID ID_INTEGER ID_RESIDENT ID_STRING

/* Other tokens */
%token EOL

/* Precedence and associativity */
%nonassoc EMPTY
%nonassoc STRING_LITERAL
%nonassoc SPC
%left EOR OR
%left AND
%left OP_EQ OP_NEQ OP_LT OP_LTE OP_GT OP_GTE
%left '+' '-'
%left '*' '/' DIV MOD
%left '^'
%nonassoc '('
%nonassoc UMINUS UNOT UPLUS

/* Nonterminal type declarations */
%type <ex> array builtin_function expr literal scalar var
%type <ex> vars opt_vars
%type <ex> exprs opt_expr opt_exprs
%type <ex> optSTEP
%type <ex> file_ptr

%type <pi> ilist ivar ivars prompt_items prompt_list
%type <pi> prompt_prefix prompt_prefixes prompt_value
%type <pi> optp_apos opt_plist optp_semi optp_spec
%type <pi> plist plist_expr plist_item plist_items

%type <st> assignment fn_return_stmt if_stmt optELSE
%type <st> stmt stmt_list opt_stmt
%type <st> tail_stmt line_stmts

%type <v> data_item data_items

%%

/* Lines and lists of statements */
start: line_list
 ;

line_list:
   line
 | line_list EOL line
 ;

line:                               /* ignore empty lines */
 | INT_LITERAL line_stmts           {
                                        if ( line_add($1, $2) != STATUS_OK ) {
                                            yyerror("duplicate line %d", $1);
                                            YYABORT;
                                        }
                                    }
 | error                            {
                                        yyerror("line %d: fatal error", @1);
                                        YYABORT;
                                    }
 ;

line_stmts:                         { $$ = statement_null_new(); }
 | fn_return_stmt                   { $$ = $1; }
 | tail_stmt                        { $$ = $1; }
 | stmt_list                        { $$ = $1; }
 | stmt_list ':' tail_stmt          { $$ = statement_append($1, $3); }
 ;

 /* TODO(paul): IF statements are currently only allowed once in a
  * line and at the end, to avoid the conflicts that arise from a
  * rule including a statement_list being allowed to appear inside
  * a statement list. Lines can indeed contain more than one IF
  * statement in BBC BASIC II, so we need to fix this. The "dangling
  * else" is one of the two canonical problems that are usually solved
  * by precedence rules, so maybe we'll have to resort to that.
  */
tail_stmt:
   if_stmt                          { $$ = $1; }
 | ON expr GOSUB exprs optELSE      { $$ = statement_on_gosub_new($2, $4, $5); }
 | ON expr GOTO exprs optELSE       { $$ = statement_on_goto_new($2, $4, $5); }
 ;

stmt_list:
   stmt                             { $$ = $1; }
 | stmt_list ':' stmt               { $$ = statement_append($1, $3); }
 ;

opt_stmt:                           { $$ = NULL; }
 | stmt                             { $$ = $1; }
 ;


/* Statements */
stmt:
   BPUT var ',' expr                { $$ = statement_bput_new($2, $4); }
 | CLEAR                            { $$ = statement_clear_new(); }
 | CLOSE INT_LITERAL                { $$ = statement_close_new(expr_int_new($2)); }
 | CLOSE var                        { $$ = statement_close_new($2); }
 | COLOUR expr                      { $$ = statement_colour_new($2); }
 | DATA data_items                  { $$ = statement_data_new($2); }
 | DEF FN opt_vars                  { $$ = statement_def_fn_new($2, $3); free($2); }
 | DEF PROC opt_vars                { $$ = statement_def_proc_new($2, $3); free($2); }
 | DIM array                        { $$ = statement_dim_new($2); }
 | END                              { $$ = statement_end_new(); }
 | ENDPROC                          { $$ = statement_endproc_new(); }
 | FOR assignment TO expr optSTEP   { $$ = statement_for_new($2, $4, $5); }
 | GOSUB expr                       { $$ = statement_gosub_new($2); }
 | GOTO expr                        { $$ = statement_goto_new($2); }
 | INPUT ilist                      { $$ = statement_input_new($2, false); }
 | INPUT LINE ilist                 { $$ = statement_input_new($3, true); }
 | INPUT_ var ',' vars              { $$ = statement_inputf_new($2, $4); }
 | optLET assignment                { $$ = $2; }
 | LOCAL vars                       { $$ = statement_local_new($2); }
 | NEXT                             { $$ = statement_next_new(NULL); }
 | NEXT var                         { $$ = statement_next_new($2); }
 | ON ERROR GOTO expr               { $$ = statement_on_error_new(statement_goto_new($4)); }
 | ON ERROR GOSUB expr              { $$ = statement_on_error_new(statement_gosub_new($4)); }
 | ON ERROR OFF                     { $$ = statement_on_error_new(NULL); }
 | PRINT opt_plist                  { $$ = statement_print_new($2); }
 | PRINT_ var ',' exprs             { $$ = statement_printf_new($2, $4); }
 | PROC opt_exprs                   { $$ = statement_proc_new($1, $2); free($1); }
 | READ vars                        { $$ = statement_read_new($2); }
 | REM                              { $$ = statement_rem_new($1); free($1); }
 | REPEAT opt_stmt                  { $$ = statement_repeat_new($2); }
 | REPORT                           { $$ = statement_report_new(); }
 | RESTORE opt_expr                 { $$ = statement_restore_new($2); }
 | RETURN                           { $$ = statement_return_new(); }
 | STOP                             { $$ = statement_stop_new(); }
 | TRACE OFF                        { $$ = statement_trace_new(TRACE_OFF, NULL); }
 | TRACE ON                         { $$ = statement_trace_new(TRACE_ON, NULL); }
 | TRACE expr                       { $$ = statement_trace_new(TRACE_ON, $2); }
 | UNTIL expr                       { $$ = statement_until_new($2); }
 ;

fn_return_stmt:
   OP_EQ expr                       { $$ = statement_fn_return_new($2); }
 ;

assignment:
   var OP_EQ expr                   { $$ = statement_assign_new($1, $3); }
 | file_ptr OP_EQ expr              { $$ = statement_assign_new($1, $3); }
 ;

if_stmt:
   IF expr optTHEN
       stmt_list optELSE            { $$ = statement_if_new($2, $4, $5); }
 ;


/* Optional statements */
optELSE:                            { $$ = NULL; }
 | ELSE stmt_list                   { $$ = $2; }
 ;

optSTEP:                            { $$ = expr_int_new(1); }
 | STEP expr                        { $$ = $2; }
 ;


/* Optional keywords */
optLET:                             /* just an optional keyword */
 | LET
 ;

optTHEN:                            /* just an optional keyword */
 | THEN
 ;


/* PRINT list rules */
opt_plist:                          { $$ = NULL; }
 | plist                            { $$ = $1; }
 ;

plist:
   plist_items optp_apos
   optp_semi                        {
                                        struct print_item * pi;
                                        pi = print_list_item_append($1, $2);
                                        $$ = print_list_item_append(pi, $3);
                                    }
 ;

plist_items:
   plist_item                       { $$ = $1; }
 | plist_items plist_item           { $$ = print_list_item_append($1, $2); }
 ;

plist_item:
   optp_apos
   optp_spec plist_expr %prec EMPTY {
                                        struct print_item * pi;
                                        pi = print_list_item_append($1, $2);
                                        $$ = print_list_item_append(pi, $3);
                                    }
 ;

plist_expr:
   expr %prec EMPTY                 { $$ = print_list_expr_append(NULL, $1); }
 | SPC '(' expr ')'                 { $$ = print_list_expr_append(NULL, expr_func_spc_new($3)); }
 ;

optp_semi:                          { $$ = NULL; }
 | ';'                              { $$ = print_list_specifier_append(NULL, PRINT_SEMICOLON); }
 ;

optp_spec:                          { $$ = NULL; }
 | ','                              { $$ = print_list_specifier_append(NULL, PRINT_COMMA); }
 | ';'                              { $$ = print_list_specifier_append(NULL, PRINT_SEMICOLON); }
 ;

optp_apos:                          { $$ = NULL; }
 | '\''                             { $$ = print_list_specifier_append(NULL, PRINT_APOSTROPHE); }
 ;


/* INPUT list rules */
ilist:
   ivars prompt_list                { $$ = print_list_item_append($1, $2); }
 | prompt_list                      { $$ = $1; }
 ;

prompt_list: %prec EMPTY            { $$ = NULL; }
 | prompt_items                     { $$ = $1; }
 | prompt_list prompt_items         { $$ = print_list_item_append($1, $2); }
 ;

prompt_items:
   prompt_prefixes ivars            { $$ = print_list_item_append($1, $2); }
 ;
   
prompt_prefixes:
   prompt_value                     { $$ = $1; }
 | prompt_prefixes prompt_prefix    { $$ = print_list_item_append($1, $2); }
 ;

prompt_prefix:
   prompt_value                    { $$ = $1; }
 | ',' prompt_value                { $$ = print_list_item_append(print_list_specifier_append(NULL, PRINT_COMMA), $2); }
 | ';' prompt_value                { $$ = print_list_item_append(print_list_specifier_append(NULL, PRINT_SEMICOLON), $2); }
 ;

prompt_value:
   STRING_LITERAL                   { $$ = print_list_expr_append(NULL, expr_string_new($1)); free($1);}
 | SPC '(' expr ')'                 { $$ = print_list_expr_append(NULL, expr_func_spc_new($3)); }
 ;

ivars:
   ivar                             { $$ = $1; }
 | ivars ivar                       { $$ = print_list_item_append($1, $2); }
 ;

ivar:
   var                              { $$ = print_list_expr_append(NULL, $1); }
 | ',' var                          { $$ = print_list_expr_append(print_list_specifier_append(NULL, PRINT_COMMA), $2); }
 | ';' var                          { $$ = print_list_expr_append(print_list_specifier_append(NULL, PRINT_SEMICOLON), $2); }
 ;


/* Lists and optional lists of expressions */
opt_exprs: %prec EMPTY              { $$ = NULL; }
 | '(' exprs ')'                    { $$ = $2; }
 ;

opt_expr:                           { $$ = NULL; }
 | expr                             { $$ = $1; }
 ;

exprs:
   expr                             { $$ = $1; }
 | exprs ',' expr                   { $$ = expr_append($1, $3); }
 ;

opt_vars:                           { $$ = NULL; }
 | '(' vars ')'                     { $$ = $2; }
 ;

vars:
   var                              { $$ = $1; }
 | vars ',' var                     { $$ = expr_append($1, $3); }
 ;

data_items:
   data_item                        { $$ = $1; }
 | data_items ',' data_item         { $$ = value_append($1, $3); }
 ;


/* Expressions */
expr:
   expr OR expr                     { $$ = expr_op_or_new($1, $3); }
 | expr EOR expr                    { $$ = expr_op_eor_new($1, $3); }
 | expr AND expr                    { $$ = expr_op_and_new($1, $3); }
 | expr OP_EQ expr                  { $$ = expr_op_eq_new($1, $3); }
 | expr OP_NEQ expr                 { $$ = expr_op_neq_new($1, $3); }
 | expr OP_GT expr                  { $$ = expr_op_gt_new($1, $3); }
 | expr OP_GTE expr                 { $$ = expr_op_gte_new($1, $3); }
 | expr OP_LT expr                  { $$ = expr_op_lt_new($1, $3); }
 | expr OP_LTE expr                 { $$ = expr_op_lte_new($1, $3); }
 | expr '+' expr                    { $$ = expr_op_add_new($1, $3); }
 | expr '-' expr                    { $$ = expr_op_sub_new($1, $3); }
 | expr '*' expr                    { $$ = expr_op_mul_new($1, $3); }
 | expr '/' expr                    { $$ = expr_op_div_new($1, $3); }
 | expr DIV expr                    { $$ = expr_op_idiv_new($1, $3); }
 | expr MOD expr                    { $$ = expr_op_mod_new($1, $3); }
 | expr '^' expr                    { $$ = expr_op_exp_new($1, $3); }
 | '-' expr %prec UMINUS            { $$ = expr_op_uminus_new($2); }
 | '+' expr %prec UPLUS             { $$ = $2; }
 | NOT expr %prec UNOT              { $$ = expr_op_not_new($2); }
 | FN opt_exprs                     { $$ = expr_fn_new($1, $2); free($1); }
 | '(' expr ')'                     { $$ = $2; }
 | builtin_function                 { $$ = $1; }
 | literal                          { $$ = $1; }
 | var                              { $$ = $1; }
 ;

builtin_function:
   ABS '(' expr ')'                 { $$ = expr_func_abs_new($3); }
 | ACS '(' expr ')'                 { $$ = expr_func_acs_new($3); }
 | ASC '(' expr ')'                 { $$ = expr_func_asc_new($3); }
 | ASN '(' expr ')'                 { $$ = expr_func_asn_new($3); }
 | ATN '(' expr ')'                 { $$ = expr_func_atn_new($3); }
 | BGET var                         { $$ = expr_func_bget_new($2); }
 | CHRS '(' expr ')'                { $$ = expr_func_chrs_new($3); }
 | COS '(' expr ')'                 { $$ = expr_func_cos_new($3); }
 | DEG '(' expr ')'                 { $$ = expr_func_deg_new($3); }
 | EOF_ '(' var ')'                 { $$ = expr_func_eof_new($3); }
 | ERL                              { $$ = expr_func_erl_new(); }
 | ERR                              { $$ = expr_func_err_new(); }
 | EXP '(' expr ')'                 { $$ = expr_func_exp_new($3); }
 | EXT '(' var ')'                  { $$ = expr_func_ext_new($3); }
 | GET                              { $$ = expr_func_get_new(); }
 | GETS                             { $$ = expr_func_gets_new(); }
 | INKEY '(' expr ')'               { $$ = expr_func_inkey_new($3); }
 | INKEYS '(' expr ')'              { $$ = expr_func_inkeys_new($3); }
 | INSTR '(' expr ',' expr ')'      { $$ = expr_func_instr_new($3, $5, NULL); }
 | INSTR '(' expr ',' expr ','
       expr ')'                     { $$ = expr_func_instr_new($3, $5, $7); }
 | INT '(' expr ')'                 { $$ = expr_func_int_new($3); }
 | LEFTS '(' expr ',' expr ')'      { $$ = expr_func_lefts_new($3, $5); }
 | LEN  '(' expr ')'                { $$ = expr_func_len_new($3); }
 | LN  '(' expr ')'                 { $$ = expr_func_ln_new($3); }
 | LOG '(' expr ')'                 { $$ = expr_func_log_new($3); }
 | MIDS '(' expr ',' expr ')'       { $$ = expr_func_mids_new($3, $5, NULL); }
 | MIDS '(' expr ',' expr ','
       expr ')'                     { $$ = expr_func_mids_new($3, $5, $7); }
 | OPENIN '(' expr ')'              { $$ = expr_func_openin_new($3); }
 | OPENOUT '(' expr ')'             { $$ = expr_func_openout_new($3); }
 | OPENUP '(' expr ')'              { $$ = expr_func_openup_new($3); }
 | file_ptr                         { $$ = $1; }
 | RAD '(' expr ')'                 { $$ = expr_func_rad_new($3); }
 | RIGHTS '(' expr ',' expr ')'     { $$ = expr_func_rights_new($3, $5); }
 | RND %prec EMPTY                  { $$ = expr_func_rnd_new(NULL); }
 | RND '(' expr ')'                 { $$ = expr_func_rnd_new($3); }
 | SGN '(' expr ')'                 { $$ = expr_func_sgn_new($3); }
 | SIN '(' expr ')'                 { $$ = expr_func_sin_new($3); }
 | SQR '(' expr ')'                 { $$ = expr_func_sqr_new($3); }
 | STRINGS '(' expr ',' expr ')'    { $$ = expr_func_strings_new($3, $5); }
 | STRS '(' expr ')'                { $$ = expr_func_strs_new($3); }
 | TAN '(' expr ')'                 { $$ = expr_func_tan_new($3); }
 | VAL '(' expr ')'                 { $$ = expr_func_val_new($3); }
 ;

file_ptr:
   PTR var                          { $$ = expr_func_ptr_new($2); }
 ;

data_item:
   FLOAT_LITERAL                    { $$ = value_float_new($1); }
 | HEX_LITERAL                      { $$ = value_int_new($1); }
 | INT_LITERAL                      { $$ = value_int_new($1); }
 | STRING_LITERAL                   { $$ = value_string_new($1); free($1); }
 ;

literal:
   FALSE                            { $$ = expr_int_new($1); }
 | FLOAT_LITERAL                    { $$ = expr_float_new($1); }
 | HEX_LITERAL                      { $$ = expr_int_new($1); }
 | INT_LITERAL                      { $$ = expr_int_new($1); }
 | PI                               { $$ = expr_float_new(3.14159265359); }
 | STRING_LITERAL                   { $$ = expr_string_new($1); free($1); }
 | TRUE                             { $$ = expr_int_new($1); }
 ;

var:
   scalar
 | array
 ;

scalar:
   ID %prec EMPTY                   { $$ = expr_variable_new($1); free($1); }
 | ID_INTEGER %prec EMPTY           { $$ = expr_variable_new($1); free($1); }
 | ID_RESIDENT %prec EMPTY          { $$ = expr_variable_new($1); free($1); }
 | ID_STRING %prec EMPTY            { $$ = expr_variable_new($1); free($1); }
 ;

array:
   ID '(' exprs ')'                 { $$ = expr_array_new($1, $3); free($1); }
 | ID_INTEGER '(' exprs ')'         { $$ = expr_array_new($1, $3); free($1); }
 | ID_RESIDENT '(' exprs ')'        { $$ = expr_array_new($1, $3); free($1); }
 | ID_STRING '(' exprs ')'          { $$ = expr_array_new($1, $3); free($1); }
 ;

%%

int yyerror(char * fmt, ...) {
#if HAVE_CONFIG_H
    fprintf(stderr, "%s: ", PACKAGE);
#endif

    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fputc('\n', stderr);

    return 0;
}
