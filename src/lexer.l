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

 /* bbasic lexer */

%option noyywrap
%option noinput
%option nounput
%option yylineno

%x STATE_REM
%x STATE_DATA

DEC_INT     (0|[1-9][0-9]*)   
FLOAT       (([0-9]*\.?[0-9]+|[0-9]+\.)([Ee][-+]?[0-9]+)?)
HEX_INT     ([0-9a-fA-F]+)
ID_CHAR     ([a-zA-Z0-9_])
ALPHA       ([A-Za-z])
QUOTED      (\"(\\.|[^"\\])*\")

%{
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include "parser.h"
#include "util.h"

double convert_double(const char * s);
int32_t convert_int(const char * s, const int base);
char * strip_quotes(const char * s);

int yyerror(char * s, ...);
#define YY_USER_ACTION yylloc.first_line = yylloc.last_line = yylineno;
%}

%%

 /* Keywords */
"ABS"                   { return ABS; }
"ACS"                   { return ACS; }
"AND"                   { return AND; }
"ASC"                   { return ASC; }
"ASN"                   { return ASN; }
"ATN"                   { return ATN; }
"AUTO"                  { return AUTO; }
"BGET#"                 { return BGET; }
"BPUT#"                 { return BPUT; }
"CHAIN"                 { return CHAIN; }
"CHR$"                  { return CHRS; }
"CLEAR"                 { return CLEAR; }
"CLOSE#"                { return CLOSE; }
"COLOUR"                { return COLOUR; }
"COS"                   { return COS; }
"DEF"                   { return DEF; }
"DEG"                   { return DEG; }
"DELETE"                { return DELETE; }
"DIM"                   { return DIM; }
"DIV"                   { return DIV; }
"ELSE"                  { return ELSE; }
"END"                   { return END; }
"ENDPROC"               { return ENDPROC; }
"EOF#"                  { return EOF_; }
"EOR"                   { return EOR; }
"ERL"                   { return ERL; }
"ERR"                   { return ERR; }
"ERROR"                 { return ERROR; }
"EVAL"                  { return EVAL; }
"EXP"                   { return EXP; }
"EXT#"                  { return EXT; }
"FOR"                   { return FOR; }
"FALSE"                 { yylval.n = 0; return FALSE; }
"GET"                   { return GET; }
"GET$"                  { return GETS; }
"GOSUB"                 { return GOSUB; }
"GOTO"                  { return GOTO; }
"IF"                    { return IF; }
"INKEY"                 { return INKEY; }
"INKEY$"                { return INKEYS; }
"INPUT"                 { return INPUT; }
"INPUT#"                { return INPUT_; }
"INSTR"                 { return INSTR; }
"INT"                   { return INT; }
"LEFT$"                 { return LEFTS; }
"LEN"                   { return LEN; }
"LET"                   { return LET; }
"LINE"                  { return LINE; }
"LIST"                  { return LIST; }
"LISTO"                 { return LISTO; }
"LN"                    { return LN; }
"LOAD"                  { return LOAD; }
"LOCAL"                 { return LOCAL; }
"LOG"                   { return LOG; }
"MID$"                  { return MIDS; }
"MOD"                   { return MOD; }
"NEW"                   { return NEW; }
"NEXT"                  { return NEXT; }
"NOT"                   { return NOT; }
"OFF"                   { return OFF; }
"OLD"                   { return OLD; }
"ON"                    { return ON; }
"OPENIN"                { return OPENIN; }
"OPENOUT"               { return OPENOUT; }
"OPENUP"                { return OPENUP; }
"OR"                    { return OR; }
"PI"                    { return PI; }
"PRINT"                 { return PRINT; }
"PRINT#"                { return PRINT_; }
"PTR#"                  { return PTR; }
"RAD"                   { return RAD; }
"READ"                  { return READ; }
"RENUMBER"              { return RENUMBER; }
"REPEAT"                { return REPEAT; }
"REPORT"                { return REPORT; }
"RESTORE"               { return RESTORE; }
"RETURN"                { return RETURN; }
"RIGHT$"                { return RIGHTS; }
"RND"                   { return RND; }
"RUN"                   { return RUN; }
"SAVE"                  { return SAVE; }
"SGN"                   { return SGN; }
"SIN"                   { return SIN; }
"SPC"                   { return SPC; }
"SQR"                   { return SQR; }
"STEP"                  { return STEP; }
"STOP"                  { return STOP; }
"STR$"                  { return STRS; }
"STRING$"               { return STRINGS; }
"TAN"                   { return TAN; }
"THEN"                  { return THEN; }
"TO"                    { return TO; }
"TRACE"                 { return TRACE; }
"TRUE"                  { yylval.n = -1; return TRUE; }
"UNTIL"                 { return UNTIL; }
"VAL"                   { return VAL; }

 /* DATA items */
"DATA"                  { BEGIN STATE_DATA; return DATA; }
<STATE_DATA>\n          { BEGIN INITIAL; return EOL; }
<STATE_DATA>[ \t]+      /* ignore whitespace */
<STATE_DATA>","         { return ','; }
<STATE_DATA>{QUOTED}    {
                            yylval.s = strip_quotes(yytext);
                            return STRING_LITERAL;
                        }
<STATE_DATA>{DEC_INT}   {
                            yylval.n = convert_int(yytext, 10);
                            return INT_LITERAL;
                        }
<STATE_DATA>&{HEX_INT}  {
                            yylval.n = convert_int(yytext+1, 16);
                            return INT_LITERAL;
                        }
<STATE_DATA>{FLOAT}     {
                            yylval.d = convert_double(yytext);
                            return FLOAT_LITERAL;
                        }
<STATE_DATA>[^ \t,\n][^,\n]* {
                            yylval.s = x_strdup(yytext);
                            return STRING_LITERAL;
                        }

 /* Remarks */
"REM"                   { BEGIN STATE_REM; }
<STATE_REM>.*$          {
                            yylval.s = x_strdup(yytext);
                            BEGIN INITIAL;
                            return REM;
                        }

 /* Identifiers */
"FN"{ID_CHAR}+          {
                            yylval.s = x_strdup(yytext);
                            return FN;
                        }
"PROC"{ID_CHAR}+        {
                            yylval.s = x_strdup(yytext);
                            return PROC;
                        }
[A-Z@]%                 {
                            yylval.s = x_strdup(yytext);
                            return ID_RESIDENT;
                        }
[a-z]%                  {
                            yylval.s = x_strdup(yytext);
                            return ID_INTEGER;
                        }
{ALPHA}{ID_CHAR}+%      {
                            yylval.s = x_strdup(yytext);
                            return ID_INTEGER;
                        }
{ALPHA}{ID_CHAR}*"$"    {
                            yylval.s = x_strdup(yytext);
                            return ID_STRING;
                        }
{ALPHA}{ID_CHAR}*       {
                            yylval.s = x_strdup(yytext);
                            return ID;
                        }

 /* Expression literals */
{QUOTED}                {
                            yylval.s = strip_quotes(yytext);
                            return STRING_LITERAL;
                        }
{DEC_INT}               {
                            yylval.n = convert_int(yytext, 10);
                            return INT_LITERAL;
                        }
&{HEX_INT}              {
                            yylval.n = convert_int(yytext+1, 16);
                            return INT_LITERAL;
                        }
{FLOAT}                 {
                            yylval.d = convert_double(yytext);
                            return FLOAT_LITERAL;
                        }

 /* Operators */
"="                     { return OP_EQ; }
"<>"                    { return OP_NEQ; }
"<"                     { return OP_LT; }
"<="                    { return OP_LTE; }
">"                     { return OP_GT; }
">="                    { return OP_GTE; }

 /* End-of-line */
\n                      { return EOL; }

 /* Ignores */
[ \t]+                  /* ignore whitespace */

 /* Return any unrecognized characters */
.                       { return yytext[0]; }

%%

/* Converts a string to a double */
double
convert_double(const char * s) {
    errno = 0;
    char * endptr;
    double d = strtod(s, &endptr);
    if ( *endptr || errno ) {
        yyerror("float out of range");
        yyterminate();
    }

    return d;
}

/* Converts a string to an integer */
int32_t
convert_int(const char * s, const int base) {
    errno = 0;
    char * endptr;
    long int n = strtol(s, &endptr, base);
    if ( *endptr || errno || n < INT32_MIN || n > INT32_MAX ) {
        yyerror("integer out of range");
        yyterminate();
    }

    return (int32_t) n;
}

/* Strips the leading and trailing quote from a quoted string */
char *
strip_quotes(const char * s) {
    char * tr = x_strdup(s+1);
    tr[strlen(tr)-1] = '\0';

    return tr;
}
