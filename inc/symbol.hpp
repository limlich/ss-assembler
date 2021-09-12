#ifndef SYMBOL_H
#define SYMBOL_H

#include <unordered_map>
#include <string>
#include <vector>

#include "types.hpp"

class Symbol;

struct RelEntry
{
    RelEntry()
    {}
    RelEntry(const Symbol *symbol, const std::string& section, ushort location) :
        symbol(symbol), section(section), location(location)
    {}

    const Symbol *symbol; // extern symbol or location dependent local symbol (label)
    std::string section; // fixup section name
    ushort location; // fixup address within section
};

enum SymbolBind: ubyte
{
    SYMB_LOCAL,
    SYMB_GLOBAL
};

enum SymbolType: ubyte
{
    SYMT_UNDEF,  // external symbols
    SYMT_ABS,    // no relocation
    SYMT_LABEL,  // relocatable
    SYMT_SECTION // section names for relocation (LOCAL binding)
};

struct SymbolEntry
{
    SymbolEntry() :
        type(SYMT_UNDEF)
    {}
    SymbolEntry(SymbolBind bind, SymbolType type, ushort value, uint nameOffset, uint sectionEntryId) :
        bind(bind), type(type), value(value), nameOffset(nameOffset), sectionEntryId(sectionEntryId)
    {}

    SymbolBind bind;
    SymbolType type;
    ushort value;
    uint nameOffset;
    uint sectionEntryId;
};

struct Symbol
{
    Symbol() :
        global(false), external(false), label(false), defined(false), value(0x00u), section(""), entry(nullptr)
    {}

    bool global; // .global
    bool external; // .extern
    bool label; // location dependent
    bool defined;
    ushort value;
    std::string section;
    SymbolEntry *entry;
};

typedef std::unordered_map<std::string, Symbol> SymbolTable;

#endif