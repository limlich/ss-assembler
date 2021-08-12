#include "assembler.hpp"

int main(int argc, char *argv[])
{
    Assembler assembler;
    if (argc > 1)
        return assembler.run(argv[1]);
    return 0;
}
