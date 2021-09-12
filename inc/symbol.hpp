#ifndef SYMBOL_H
#define SYMBOL_H

#include <unordered_map>
#include <string>
#include <vector>

#include "types.hpp"

class Symbol;

struct RelEntry
{
    RelEntry(ushort offset, uint symbolId) :
        offset(offset), symbolId(symbolId)
    {}

    ushort offset;
    uint symbolId;
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
        bind(SYMB_LOCAL), type(SYMT_UNDEF), value(0)
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
        global(false), external(false), section(""), id(0)
    {}

    bool isDeclared() const { return !(entry.bind == SYMB_LOCAL && entry.type == SYMT_UNDEF); }
    bool isDefined() const { return entry.type != SYMT_UNDEF; }
    bool isAbs() const { return entry.type == SYMT_ABS; }
    bool isLabel() const { return entry.type == SYMT_LABEL; }
    bool isSection() const { return entry.type == SYMT_SECTION; }

    bool global;
    bool external;
    SymbolEntry entry;
    std::string section;
    uint id; // symbol table entry id
};

typedef std::unordered_map<std::string, Symbol> SymbolMap;
typedef std::vector<SymbolEntry> SymbolTable;

#endif