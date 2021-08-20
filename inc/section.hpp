#ifndef SECTION_H
#define SECTION_H

#include <unordered_map>
#include <string>
#include <vector>

#include "types.hpp"
#include "symbol.hpp"

struct Section
{
    std::vector<ubyte> data;
    std::vector<RelEntry> relEntries;
};

typedef std::unordered_map<std::string, Section> SectionTable;

#endif