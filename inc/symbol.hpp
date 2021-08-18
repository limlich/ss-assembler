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
    Symbol()
    {}
    Symbol(const std::string& section, ushort value, bool imp, bool exp, bool rel) :
        section(section), value(value), imp(imp), exp(exp), rel(rel)
    {}

    std::string section;
    ushort value;
    bool imp; // global
    bool exp; // export
    bool rel; // position dependent
    std::vector<RelEntry>  relEntries;
};

typedef std::unordered_map<std::string, Symbol> symbol_table;

#endif