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
    ST_SYM_TAB // section containing symbol table entries
};

const std::string SECTION_PREFIX = "."; // section symbol prefix
const std::string REL_SUFFIX = ".rel"; // relocation section suffix
const std::string STR_SECTION = SECTION_PREFIX + "names.str"; // names section name
const std::string SYM_TAB_SECTION = SECTION_PREFIX + "sym.tab"; // symbol table section name

struct SectionEntry
{
    SectionEntry(SectionType type = ST_NONE) :
        type(type), nameOffset(0), dataOffset(0), size(0)
    {}

    SectionType type;
    ushort nameOffset; // offset in .str section
    uint dataOffset; // section data offset
    ushort size; // section size in bytes
};

struct Section
{
    Section() :
        id(0)
    {}
    Section(SectionType type) :
        entry(type)
    {}

    SectionEntry entry;
    std::vector<ubyte> data;
    ushort id; // section header table entry index
};

typedef std::unordered_map<std::string, Section> SectionMap;
typedef std::vector<SectionEntry> SectionHeaderTable;

#endif