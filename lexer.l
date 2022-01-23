%option c++
%option noyywrap
%option nodefault
%option yyclass="Lexer"

%{
#include <cstdlib>
#include "lexer.hpp"
#include "parser.hpp"
#include "assembler.hpp"

#define YY_USER_ACTION assembler.locationAddColumns(yyleng);

void yy::Lexer::skip_line(Assembler& assembler)
{
    yy::Parser::symbol_kind_type symKind;
    do {
        symKind = get_token(assembler).kind();
    } while (symKind != yy::Parser::symbol_kind_type::S_NEWLINE &&
             symKind != yy::Parser::symbol_kind_type::S_YYEOF);
}

%}

ident       [a-zA-Z_][a-zA-Z0-9_]*
int_10      [0-9]+
int_16      0[xX][0-9a-fA-F]+
reg         (r[0-9]+)|(sp)|(pc)|(psw)
dollar      \$
percent     %
colon       :
comma       ,
period      \.
plus        \+
mul         \*
sbr_open    \[
sbr_close   \]
newline     \n

%%
{reg}       { return yy::Parser::make_REG(yytext, assembler.getLocation()); }
{ident}     { return yy::Parser::make_IDENT(yytext, assembler.getLocation()); }
{int_10}    { return yy::Parser::make_INT_10(yytext, assembler.getLocation()); }
{int_16}    { return yy::Parser::make_INT_16(yytext, assembler.getLocation()); }
{dollar}    { return yy::Parser::make_DOLLAR(assembler.getLocation()); }
{percent}   { return yy::Parser::make_PERCENT(assembler.getLocation()); }
{colon}     { return yy::Parser::make_COLON(assembler.getLocation()); }
{comma}     { return yy::Parser::make_COMMA(assembler.getLocation()); }
{period}    { return yy::Parser::make_PERIOD(assembler.getLocation()); }
{plus}      { return yy::Parser::make_PLUS(assembler.getLocation()); }
{mul}       { return yy::Parser::make_MUL(assembler.getLocation()); }
{sbr_open}  { return yy::Parser::make_SBR_OPEN(assembler.getLocation()); }
{sbr_close} { return yy::Parser::make_SBR_CLOSE(assembler.getLocation()); }
{newline}   { assembler.locationAddLines(); return yy::Parser::make_NEWLINE(assembler.getLocation()); }
<<EOF>>     { return yy::Parser::make_YYEOF(assembler.getLocation()); }
[ \t\r]     {}
#.*         {}
.           { return yy::Parser::make_YYUNDEF(assembler.getLocation()); }
%%
