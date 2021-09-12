#ifndef OBJ_H
#define OBJ_H

#include "types.hpp"
#include "section.hpp"

struct ObjHeader
{
    const ubyte magic[6] = { 0x4, 0xF, 'O', 'B', 'J', '\0' };
    ubyte objHeaderSize = sizeof(ObjHeader);
    ubyte shtEntrySize = sizeof(SectionEntry);
    uint shtOffset; // section header table offset in obj file
    uint shtSize; // section header table size (in entries)
    uint strEntryId; // entry of names (.str) section in section header table
};

#endif