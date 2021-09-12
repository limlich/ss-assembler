#ifndef SECTION_H
#define SECTION_H

#include <unordered_map>
#include <string>
#include <vector>

#include "types.hpp"
#include "symbol.hpp"

enum SectionType: ubyte
{
    ST_NONE = 0,
    ST_DATA,   // code and data
    ST_REL,    // section containing relocation entries
    ST_STR, // section containing symbol identifiers
};

struct SectionEntry
{
    SectionEntry() :
        type(ST_NONE)
    {}

    SectionType type;
    ushort nameOffset; // offset in .str section
    uint dataOffset; // section data offset
};

struct Section
{
    Section() :
        entry(nullptr)
    {}

    std::vector<ubyte> data;
    SectionEntry *entry;
};

typedef std::unordered_map<std::string, Section> SectionTable;

#endif