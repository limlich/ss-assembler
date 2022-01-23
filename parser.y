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

#define PARSER_CALLBACK(X) { int r = X; if (r != AE_OK && r != AE_SYNTAX_NOSKIP) return r; }
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

%token <std::string> IDENT "identifier"
%token <std::string> INT_10 "integer10"
%token <std::string> INT_16 "integer16"
%token <std::string> REG "register"
%token DOLLAR "$"
%token PERCENT "%"
%token COLON ":"
%token COMMA ","
%token PERIOD "."
%token PLUS "+"
%token MUL "*"
%token SBR_OPEN "["
%token SBR_CLOSE "]"
%token NEWLINE "newline"
%token YYEOF 0 "end of file"
%token YYUNDEF

%type <std::string> label
%type <ushort> literal

%%
asm:  stmt
    | asm NEWLINE stmt

stmt: %empty
    | instr
    | dir
    | label
    | label instr
    | label dir

instr: IDENT { PARSER_CALLBACK(assembler.instr($1)) }
    |  IDENT instr_arg { PARSER_CALLBACK(assembler.instr($1)) }
    |  IDENT instr_arg COMMA instr_arg { PARSER_CALLBACK(assembler.instr($1)) }

instr_arg: DOLLAR literal                          /* $<literal>           */
           { PARSER_CALLBACK(assembler.instrArgImmed($2)) }
    |      DOLLAR IDENT                            /* $<symbol>            */
           { PARSER_CALLBACK(assembler.instrArgImmed($2)) }
    |      literal                                 /* <literal>            */
           { PARSER_CALLBACK(assembler.instrArgMemDirOrJmpImmed($1)) }
    |      IDENT                                   /* <symbol>             */
           { PARSER_CALLBACK(assembler.instrArgMemDirOrJmpImmed($1)) }
    |      PERCENT IDENT                           /* %<symbol>            */
           { PARSER_CALLBACK(assembler.instrArgPCRel($2)) }
    |      REG                                     /* <reg>                */
           { PARSER_CALLBACK(assembler.instrArgRegDir($1)) }
    |      SBR_OPEN REG SBR_CLOSE                  /* [<reg>]              */
           { PARSER_CALLBACK(assembler.instrArgRegInd($2)) }
    |      SBR_OPEN REG PLUS literal SBR_CLOSE     /* [<reg> + <literal>]  */
           { PARSER_CALLBACK(assembler.instrArgRegIndOff($2, $4)) }
    |      SBR_OPEN REG PLUS IDENT SBR_CLOSE       /* [<reg> + <symbol>]   */
           { PARSER_CALLBACK(assembler.instrArgRegIndOff($2, $4)) }
    |      MUL literal                             /* *<literal>           */
           { PARSER_CALLBACK(assembler.instrArgMemDirOrJmpImmed($2, true)) }
    |      MUL IDENT                               /* *<symbol>            */
           { PARSER_CALLBACK(assembler.instrArgMemDirOrJmpImmed($2, true)) }
    |      MUL REG                                 /* *<reg>               */
           { PARSER_CALLBACK(assembler.instrArgRegDir($2, true)) }
    |      MUL SBR_OPEN REG SBR_CLOSE              /* *[<reg>]             */
           { PARSER_CALLBACK(assembler.instrArgRegInd($3, true)) }
    |      MUL SBR_OPEN REG PLUS literal SBR_CLOSE /* *[<reg> + <literal>] */
           { PARSER_CALLBACK(assembler.instrArgRegIndOff($3, $5, true)) }
    |      MUL SBR_OPEN REG PLUS IDENT SBR_CLOSE   /* *[<reg> + <symbol>]  */
           { PARSER_CALLBACK(assembler.instrArgRegIndOff($3, $5, true)) }

dir:  PERIOD IDENT { PARSER_CALLBACK(assembler.dir($2)) }
    | PERIOD IDENT dir_arg_list { PARSER_CALLBACK(assembler.dir($2)) }

dir_arg_list: dir_arg
    | dir_arg_list COMMA dir_arg

dir_arg: IDENT { PARSER_CALLBACK(assembler.dirArg($1)) }
    |    literal { PARSER_CALLBACK(assembler.dirArg($1)) }

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

label: IDENT COLON { PARSER_CALLBACK(assembler.label($1)); }
%%

#include <iostream>

void yy::Parser::error(const yy::Parser::location_type& loc, const std::string& msg)
{
	std::cout << *loc.begin.filename << ":" << loc.begin.line << ":" << loc.begin.column  << ": " << msg << std::endl;
}
