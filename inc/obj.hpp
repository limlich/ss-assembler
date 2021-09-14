#ifndef OBJ_H
#define OBJ_H

#include "types.hpp"
#include "section.hpp"

const uint OBJ_MAGIC_BYTES = 0x4A424F7F; // { 0x7F, 'O', 'B', 'J' }

struct ObjHeader
{
    uint magic = OBJ_MAGIC_BYTES;
    //const ubyte OBJ_HEADER_SIZE = sizeof(ObjHeader);
    //const ubyte SHT_ENTRY_SIZE = sizeof(SectionEntry);
    uint shtOffset = 0; // section header table offset in obj file
    ushort shtSize = 0; // section header table size (in entries)
    ushort strEntryId = 0; // entry of names section in section header table
};

#endif