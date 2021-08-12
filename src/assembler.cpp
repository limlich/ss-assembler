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