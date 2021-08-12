#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "lexer.hpp"
#include "parser.hpp"

class Assembler
{
public:
    Assembler();

    int run(const std::string& filename);

    const yy::location& getLocation() const { return location_; }

    // Set begin location to current end location and advance end location by count columns (locate new token)
    void locationAddColumns(yy::location::counter_type count);
    // Advance end location by count lines
    void locationAddLines(yy::location::counter_type count = 1);

    friend class yy::Lexer;
    friend class yy::Parser;
private:
    yy::Lexer lexer_;
    yy::Parser parser_;
    yy::location location_;
};

#endif