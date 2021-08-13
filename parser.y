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
%type <unsigned long> literal

%%
program: %empty
    |    program line

line: NEWLINE
    | instr endline
    | dir endline
    | label endline
    | label instr endline
    | label dir endline

instr: IDENT { assembler.instr($1); }
    |  IDENT instr_arg { assembler.instr($1); }
    |  IDENT instr_arg COMMA instr_arg { assembler.instr($1); }

instr_arg: DOLLAR literal                          /* $<literal>           */
           { assembler.instrArgImmedLit($2); }
    |      DOLLAR IDENT                            /* $<symbol>            */
           { assembler.instrArgImmedSym($2); }
    |      literal                                 /* <literal>            */
           { assembler.instrArgMemDirOrJmpImmedLit($1); }
    |      IDENT                                   /* <symbol>             */
           { assembler.instrArgMemDirOrJmpImmedSym($1); }
    |      PERCENT IDENT                           /* %<symbol>            */
           { assembler.instrArgPCRel($2); }
    |      REG                                     /* <reg>                */
           { assembler.instrArgRegDir($1); }
    |      SBR_OPEN REG SBR_CLOSE                  /* [<reg>]              */
           { assembler.instrArgRegInd($2); }
    |      SBR_OPEN REG PLUS literal SBR_CLOSE     /* [<reg> + <literal>]  */
           { assembler.instrArgRegIndLit($2, $4); }
    |      SBR_OPEN REG PLUS IDENT SBR_CLOSE       /* [<reg> + <symbol>]   */
           { assembler.instrArgRegIndSym($2, $4); }
    |      MUL literal                             /* *<literal>           */
           { assembler.instrArgMemDirOrJmpImmedLit($2, true); }
    |      MUL IDENT                               /* *<symbol>            */
           { assembler.instrArgMemDirOrJmpImmedSym($2, true); }
    |      MUL REG                                 /* *<reg>               */
           { assembler.instrArgRegDir($2, true); }
    |      MUL SBR_OPEN REG SBR_CLOSE              /* *[<reg>]             */
           { assembler.instrArgRegInd($3, true); }
    |      MUL SBR_OPEN REG PLUS literal SBR_CLOSE /* *[<reg> + <literal>] */
           { assembler.instrArgRegIndLit($3, $5); }
    |      MUL SBR_OPEN REG PLUS IDENT SBR_CLOSE   /* *[<reg> + <symbol>]  */
           { assembler.instrArgRegIndSym($3, $5); }

dir:  PERIOD IDENT { assembler.dir($2); }
    | PERIOD IDENT dir_arg_list { assembler.dir($2); }

dir_arg_list: dir_arg
    | dir_arg_list COMMA dir_arg

dir_arg: IDENT { assembler.dirArgSym($1); }
    |    literal { assembler.dirArgLit($1); }

literal: INT_10 { $$ = std::stoul($1, nullptr, 10); }
    |    INT_16 { $$ = std::stoul($1, nullptr, 16); }

label: IDENT COLON { assembler.label($1); }

endline: NEWLINE
    |    YYEOF
%%

#include <iostream>

void yy::Parser::error(const yy::Parser::location_type& loc, const std::string& msg)
{
	std::cout << *loc.begin.filename << ":" << loc.begin.line << ":" << loc.begin.column  << ": " << msg << std::endl;
}
