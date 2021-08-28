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

struct Symbol
{
    Symbol() :
        global(false), external(false), label(false), defined(false), value(0x00u), section("")
    {}

    bool global; // .global
    bool external; // .extern
    bool label; // location dependent
    bool defined;
    ushort value;
    std::string section;
};

typedef std::unordered_map<std::string, Symbol> SymbolTable;

#endif