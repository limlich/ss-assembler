%require "3.2"
%language "c++"
%defines

%code requires {
#include <string>

namespace yy {
    class Lexer;
}
class Assembler;
}

%code top {
#include "lexer.hpp"
#include "parser.hpp"
#include "assembler.hpp"

yy::Parser::symbol_type yylex(yy::Lexer& lexer, Assembler& assembler)
{
    return lexer.get_token(assembler);
}
}

%param {yy::Lexer& lexer}
%param {Assembler& assembler}

%locations
%define api.location.file "location.hpp"

%define api.namespace {yy}
%define api.parser.class {Parser}
%define api.symbol.prefix {TOKEN_}
%define api.value.type variant
%define api.token.constructor
%define parse.error detailed
%define parse.lac full

%token <std::string> IDENT
%token <std::string> INT_10 INT_16
%token <std::string> REG
%token DOLLAR PERCENT COLON COMMA PERIOD PLUS MUL
%token SBR_OPEN SBR_CLOSE
%token NEWLINE

%type <std::string> label
%type <std::string> literal
%type <std::string> dir_arg

%%
program: %empty
    |    program line

line: NEWLINE
    | instr endline
    | dir endline
    | label endline
    | label instr endline
    | label dir endline

instr: IDENT
    |  IDENT instr_arg
    |  IDENT instr_arg COMMA instr_arg

instr_arg: DOLLAR literal                          /* $<literal>           */
    |      DOLLAR IDENT                            /* $<symbol>            */
    |      literal                                 /* <literal>            */
    |      IDENT                                   /* <symbol>             */
    |      PERCENT IDENT                           /* %<symbol>            */
    |      REG                                     /* <reg>                */
    |      SBR_OPEN REG SBR_CLOSE                  /* [<reg>]              */
    |      SBR_OPEN REG PLUS literal SBR_CLOSE     /* [<reg> + <literal>]  */
    |      SBR_OPEN REG PLUS IDENT SBR_CLOSE       /* [<reg> + <symbol>]   */
    |      MUL literal                             /* *<literal>           */
    |      MUL IDENT                               /* *<symbol>            */
    |      MUL REG                                 /* *<reg>               */
    |      MUL SBR_OPEN REG SBR_CLOSE              /* *[<reg>]             */
    |      MUL SBR_OPEN REG PLUS literal SBR_CLOSE /* *[<reg> + <literal>] */
    |      MUL SBR_OPEN REG PLUS IDENT SBR_CLOSE   /* *[<reg> + <symbol>]  */

dir:  PERIOD IDENT
    | PERIOD IDENT dir_arg_list

dir_arg_list: dir_arg
    | dir_arg_list COMMA dir_arg

dir_arg: IDENT { $$ = $1; }
    |    literal { $$ = $1; }

literal: INT_10 { $$ = $1; }
    |    INT_16 { $$ = $1; }

label: IDENT COLON { $$ = $1; }

endline: NEWLINE
    |    YYEOF
%%

#include <iostream>

void yy::Parser::error(const yy::Parser::location_type& loc, const std::string& msg)
{
	std::cout << *loc.begin.filename << ":" << loc.begin.line << ":" << loc.begin.column  << ": " << msg << std::endl;
}
