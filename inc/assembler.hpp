#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <unordered_map>
#include <variant>
#include <vector>
#include <string>

#include "lexer.hpp"
#include "parser.hpp"
#include "section.hpp"
#include "instruction.hpp"
#include "directive.hpp"
#include "register.hpp"
#include "symbol.hpp"
#include "types.hpp"

enum AssemblerExitCode: int
{
    AE_OK = 0,
    AE_SYNTAX = 1, // syntax error
    AE_END, // .end directive
    AE_FILE, // file errors (cannot open file, no file provided...)
    AE_SYMBOL, // symbol errors (multiple definition, not defined...)
    AE_SECTION, // section errors (no section, multiple definition)
};

class Assembler
{
public:
    Assembler();

    int run(const std::string& inFilename, const std::string& outFilename);

    const yy::location& getLocation() const { return location_; }

    // Set begin location to current end location and advance end location by count columns (locate new token)
    void locationAddColumns(yy::location::counter_type count);
    // Advance end location by count lines
    void locationAddLines(yy::location::counter_type count = 1);

    friend class yy::Lexer;
    friend class yy::Parser;

private: // Parser callbacks
    int instr(std::string instrName);
    int instrArgImmed(string_ushort_variant arg); // $<literal> | $<symbol>
    int instrArgMemDirOrJmpImmed(string_ushort_variant arg, bool jmpSyntax = false); // <lit/sym> (memdir or jmp immed) | *<lit/sym> (jmp memdir)
    int instrArgPCRel(const std::string& sym); // %<symbol>
    int instrArgRegDir(const std::string& reg, bool jmpSyntax = false); // <reg> | *<reg>
    int instrArgRegInd(const std::string& reg, bool jmpSyntax = false); // [<reg>] | *[<reg>]
    int instrArgRegIndOff(const std::string& reg, string_ushort_variant off, bool jmpSyntax = false); // [<reg> + <lit/sym>] | *[<reg> + <lit/sym>]

    int dir(const std::string& dirName);
    int dirArg(string_ushort_variant arg);

    int label(const std::string& label);

private:
    int instrFirstPass(const std::string& instrName);
    int instrSecondPass(const std::string& instrName);
    int dirFirstPass(const std::string& dirName);
    int dirSecondPass(const std::string& dirName);

    int writeToFile(const std::string& outFilename);

    int processWord(string_ushort_variant &arg);

    void syntaxError(const std::string& msg);
    void error(const std::string& msg);
    void warning(const std::string& msg);

    yy::Lexer lexer_;
    yy::Parser parser_;
    yy::location location_;

    ubyte pass_;
    uint lc_;

    // Section
    SectionTable sections_;
    std::string section_; // current section
    std::vector<ubyte>* sectionData_; // current section data

    // Instruction data
    ubyte instrNumArgs_;
    InstrArg instrArgs_[2];
    RegIndUpdateType regIndUpdate_;

    // Directive data
    std::vector<string_ushort_variant> dirArgs_;

    // Symbols
    bool labeled_;
    SymbolTable symbols_;
};

#endif