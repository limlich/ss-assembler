#include "assembler.hpp"

#include <fstream>

Assembler::Assembler() :
    lexer_(), parser_(lexer_, *this)
{}

int Assembler::run(const std::string& filename)
{
    std::ifstream file(filename, std::ios_base::in);
    if (!file.is_open()) {
        std::cout << "Cannot open file: " << filename << std::endl;
        return 1;
    }

    lexer_.switch_streams(&file);
    location_.initialize(&filename);

    int res = parser_.parse();

    location_.initialize();
    lexer_.switch_streams();
    file.close();

    return res;
}

void Assembler::locationAddColumns(yy::location::counter_type count)
{
    location_.step();
    location_.columns(count);
}
void Assembler::locationAddLines(yy::location::counter_type count)
{
    location_.lines(count);
}

void Assembler::instr(const std::string& instr)
{
}
void Assembler::instrArgImmedLit(unsigned long lit)
{
}
void Assembler::instrArgImmedSym(const std::string& sym)
{
}
void Assembler::instrArgMemDirOrJmpImmedLit(unsigned long lit, bool jmp)
{
}
void Assembler::instrArgMemDirOrJmpImmedSym(const std::string& sym, bool jmp)
{
}
void Assembler::instrArgPCRel(const std::string& sym)
{
}
void Assembler::instrArgRegDir(const std::string& reg, bool jmp)
{
}
void Assembler::instrArgRegInd(const std::string& reg, bool jmp)
{
}
void Assembler::instrArgRegIndLit(const std::string& reg, unsigned long lit, bool jmp)
{
}
void Assembler::instrArgRegIndSym(const std::string& reg, const std::string& sym, bool jmp)
{
}

void Assembler::dir(const std::string& dir)
{
}
void Assembler::dirArgLit(unsigned long lit)
{
}
void Assembler::dirArgSym(const std::string& sym)
{
}

void Assembler::label(const std::string& label)
{
}