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

private: // Parser callbacks
    void instr(const std::string& instr);
    void instrArgImmedLit(unsigned long lit); // $<literal>
    void instrArgImmedSym(const std::string& sym); // $<symbol>
    void instrArgMemDirOrJmpImmedLit(unsigned long lit, bool jmp = false); // <literal> (memdir or jmp immed) | *<literal> (jmp memdir)
    void instrArgMemDirOrJmpImmedSym(const std::string& sym, bool jmp = false); // <symbol> (memdir or jmp immed) | *<symbol> (jmp memdir)
    void instrArgPCRel(const std::string& sym); // %<literal>
    void instrArgRegDir(const std::string& reg, bool jmp = false); // <reg> | *<reg>
    void instrArgRegInd(const std::string& reg, bool jmp = false); // [<reg>] | *[<reg>]
    void instrArgRegIndLit(const std::string& reg, unsigned long lit, bool jmp = false); // [<reg> + <literal>] | *[<reg> + <literal>]
    void instrArgRegIndSym(const std::string& reg, const std::string& sym, bool jmp = false); // [<reg> + <symbol>] | *[<reg> + <symbol>]

    void dir(const std::string& dir);
    void dirArgLit(unsigned long lit);
    void dirArgSym(const std::string& sym);

    void label(const std::string& label);

private:
    yy::Lexer lexer_;
    yy::Parser parser_;
    yy::location location_;
};

#endif