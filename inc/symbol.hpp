#ifndef SYMBOL_H
#define SYMBOL_H

#include <unordered_map>
#include <string>
#include <vector>

#include "types.hpp"

class Symbol;

enum RelType: ubyte
{
    RT_SYM_16,
    RT_PC
};

struct RelEntry
{
    RelEntry(RelType type, ushort offset, uint symbolId) :
        type(type), offset(offset), symbolId(symbolId)
    {}

    RelType type;
    ushort offset;
    ushort symbolId;
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
        bind(SYMB_LOCAL), type(SYMT_UNDEF), value(0), nameOffset(0), sectionEntryId(0)
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
        global(false), external(false), used(false), section(""), id(0)
    {}

    bool defined() const { return entry.type != SYMT_UNDEF; }
    bool label() const { return entry.type == SYMT_LABEL; }
    bool abs() const { return entry.type == SYMT_ABS; }

    bool global;
    bool external;
    bool used;
    SymbolEntry entry;
    std::string section;
    uint id; // symbol table entry id
};

typedef std::unordered_map<std::string, Symbol> SymbolMap;
typedef std::vector<SymbolEntry> SymbolTable;

#endif