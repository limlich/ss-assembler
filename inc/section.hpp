#ifndef SECTION_H
#define SECTION_H

#include <unordered_map>
#include <string>
#include <vector>

#include "types.hpp"

struct Section
{
    std::vector<ubyte> data;
};

typedef std::unordered_map<std::string, Section> section_table;

#endif