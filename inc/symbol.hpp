#ifndef SYMBOL_H
#define SYMBOL_H

#include <unordered_map>
#include <string>
#include <vector>

#include "types.hpp"

struct RelEntry
{
    RelEntry()
    {}
    RelEntry(const std::string& section, ushort location) :
        section(section), location(location)
    {}

    std::string section;
    ushort location;
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
    std::vector<RelEntry>  relEntries;
};

typedef std::unordered_map<std::string, Symbol> SymbolTable;

#endif