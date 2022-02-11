#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <fstream>
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
#include "obj.hpp"

enum AssemblerExitCode: int
{
    AE_OK = 0,
    AE_SYNTAX = 1, // syntax error
    AE_SYNTAX_NOSKIP, // syntax error (no need to skip line)
    AE_END, // .end directive
    AE_FILE, // file errors (cannot open file, no file provided...)
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

    Symbol& getSymbol(const std::string &symbolName);
    const Symbol& getSectionSymbol(const std::string &sectionName);

    int processWord(string_ushort_variant &arg, bool instr);

    void writeObjHeader();

    void initRelSection();

    void initSymbolTable();
    void insertSymbolTableEntry(Symbol &symbol);
    void fillSymbolTable();
    void endSymbolTable();

    void initStrSection();
    std::size_t insertStrSectionEntry(const std::string &str);
    void endStrSection();

    void initSectionHeaderTable();
    void endSectionHeaderTable();
    void endSection();
    void insertSectionTableEntry(const std::string &sectionName, Section &section, ushort size = 0);
    void writeSection(Section &section);

    void syntaxError(const std::string& msg);
    void error(const std::string& msg);
    void warning(const std::string& msg);

    yy::Lexer lexer_;
    yy::Parser parser_;
    yy::location location_;

    std::ofstream outFile_;

    ubyte pass_;
    uint lc_;
    bool error_;

    ObjHeader objHeader_;

    // Section
    SectionMap sections_;
    SectionHeaderTable sectionHeaderTable_;
    std::string sectionName_; // current section name
    Section *section_; // current section
    // Relocation section
    std::string relSectionName_;
    Section *relSection_;
    std::vector<ubyte> sectionDataCache_;
    std::vector<ubyte> relSectionDataCache_;

    // Instruction data
    ubyte instrNumArgs_;
    InstrArg instrArgs_[2];
    RegIndUpdateType regIndUpdate_;
    bool pcRel_;

    // Directive data
    std::vector<string_ushort_variant> dirArgs_;

    // Symbols
    bool labeled_;
    SymbolMap symbols_;
};

#endif