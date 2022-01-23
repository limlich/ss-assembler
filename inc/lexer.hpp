#ifndef LEXER_H
#define LEXER_H

#include <istream>

#ifndef yyFlexLexerOnce
#undef yyFlexLexer
#define yyFlexLexer yyFlexLexer
#include <FlexLexer.h>
#endif

#include "parser.hpp"

class Assembler;

#undef YY_DECL
#define YY_DECL yy::Parser::symbol_type yy::Lexer::get_token(Assembler& assembler)

namespace yy {

    class Lexer : public yyFlexLexer
    {
    public:
        Lexer(std::istream* in = (std::istream*)0) : yyFlexLexer(in) {}
        virtual ~Lexer() {}
        yy::Parser::symbol_type get_token(Assembler& assembler);
        void skip_line(Assembler& assembler);
    };

}

#endif