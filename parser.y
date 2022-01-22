%require "3.5.1"
%language "c++"
%defines

%code requires {
#include <string>
#include "types.hpp"

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

#define RET_ON_ERROR(X) { int r = X; if (r != AE_OK) return r; }
}

%param {yy::Lexer& lexer}
%param {Assembler& assembler}

%locations
%define api.location.file "location.hpp"

%define api.namespace {yy}
%define api.parser.class {Parser}
%define api.value.type variant
%define api.token.constructor
%define filename_type {const std::string}
%define parse.error verbose
%define parse.lac full

%token <std::string> IDENT
%token <std::string> INT_10 INT_16
%token <std::string> REG
%token DOLLAR PERCENT COLON COMMA PERIOD PLUS MUL
%token SBR_OPEN SBR_CLOSE
%token NEWLINE
%token YYEOF 0 "end of file"
%token YYUNDEF

%type <std::string> label
%type <ushort> literal

%%
program: line
    |    program NEWLINE line

line: %empty
    | instr
    | dir
    | label
    | label instr
    | label dir

instr: IDENT { RET_ON_ERROR(assembler.instr($1)) }
    |  IDENT instr_arg { RET_ON_ERROR(assembler.instr($1)) }
    |  IDENT instr_arg COMMA instr_arg { RET_ON_ERROR(assembler.instr($1)) }

instr_arg: DOLLAR literal                          /* $<literal>           */
           { RET_ON_ERROR(assembler.instrArgImmed($2)) }
    |      DOLLAR IDENT                            /* $<symbol>            */
           { RET_ON_ERROR(assembler.instrArgImmed($2)) }
    |      literal                                 /* <literal>            */
           { RET_ON_ERROR(assembler.instrArgMemDirOrJmpImmed($1)) }
    |      IDENT                                   /* <symbol>             */
           { RET_ON_ERROR(assembler.instrArgMemDirOrJmpImmed($1)) }
    |      PERCENT IDENT                           /* %<symbol>            */
           { RET_ON_ERROR(assembler.instrArgPCRel($2)) }
    |      REG                                     /* <reg>                */
           { RET_ON_ERROR(assembler.instrArgRegDir($1)) }
    |      SBR_OPEN REG SBR_CLOSE                  /* [<reg>]              */
           { RET_ON_ERROR(assembler.instrArgRegInd($2)) }
    |      SBR_OPEN REG PLUS literal SBR_CLOSE     /* [<reg> + <literal>]  */
           { RET_ON_ERROR(assembler.instrArgRegIndOff($2, $4)) }
    |      SBR_OPEN REG PLUS IDENT SBR_CLOSE       /* [<reg> + <symbol>]   */
           { RET_ON_ERROR(assembler.instrArgRegIndOff($2, $4)) }
    |      MUL literal                             /* *<literal>           */
           { RET_ON_ERROR(assembler.instrArgMemDirOrJmpImmed($2, true)) }
    |      MUL IDENT                               /* *<symbol>            */
           { RET_ON_ERROR(assembler.instrArgMemDirOrJmpImmed($2, true)) }
    |      MUL REG                                 /* *<reg>               */
           { RET_ON_ERROR(assembler.instrArgRegDir($2, true)) }
    |      MUL SBR_OPEN REG SBR_CLOSE              /* *[<reg>]             */
           { RET_ON_ERROR(assembler.instrArgRegInd($3, true)) }
    |      MUL SBR_OPEN REG PLUS literal SBR_CLOSE /* *[<reg> + <literal>] */
           { RET_ON_ERROR(assembler.instrArgRegIndOff($3, $5, true)) }
    |      MUL SBR_OPEN REG PLUS IDENT SBR_CLOSE   /* *[<reg> + <symbol>]  */
           { RET_ON_ERROR(assembler.instrArgRegIndOff($3, $5, true)) }

dir:  PERIOD IDENT { RET_ON_ERROR(assembler.dir($2)) }
    | PERIOD IDENT dir_arg_list { RET_ON_ERROR(assembler.dir($2)) }

dir_arg_list: dir_arg
    | dir_arg_list COMMA dir_arg

dir_arg: IDENT { RET_ON_ERROR(assembler.dirArg($1)) }
    |    literal { RET_ON_ERROR(assembler.dirArg($1)) }

literal: INT_10 {
           uint lit = std::stoul($1, nullptr, 10);
           if (lit > 0xFFFFul) {
              error(assembler.getLocation(), "syntax error, literal value outside bounds: " + $1);
              return 1;
           }
           $$ = (ushort)lit;
         }

    |    INT_16 {
           uint lit = std::stoul($1, nullptr, 16);
           if (lit > 0xFFFFul) {
             error(assembler.getLocation(), "syntax error, literal value outside bounds: " + $1);
             return 1;
           }
           $$ = (ushort)lit;
         }

label: IDENT COLON { RET_ON_ERROR(assembler.label($1)); }
%%

#include <iostream>

void yy::Parser::error(const yy::Parser::location_type& loc, const std::string& msg)
{
	std::cout << *loc.begin.filename << ":" << loc.begin.line << ":" << loc.begin.column  << ": " << msg << std::endl;
}
