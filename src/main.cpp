#include <iostream>
#include <string>

#include "assembler.hpp"

int main(int argc, char *argv[])
{
    std::string inFilename, outFilename;

    for (int i = 1; i < argc; ++i) {
        if (argv[i] == std::string("-o")) {
            if (i < argc) {
                ++i;
                outFilename = argv[i];
            }
        } else
            inFilename = argv[i];
    }

    int res = AE_OK;

    if (inFilename.empty()) {
        std::cout << "No input file provided\n";
        res = AE_FILE;
    }
    if (outFilename.empty()) {
        std::cout << "No output file provided\n";
        res = AE_FILE;
    }

    if (res == AE_OK) {
        Assembler assembler;
        res = assembler.run(inFilename, outFilename);
    }

    return res;
}
