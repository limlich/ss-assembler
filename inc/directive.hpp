#ifndef DIRECTIVE_H
#define DIRECTIVE_H

#include <unordered_map>

#include "types.hpp"

enum DirArgFormat
{
    NONE, // .dir
    SYM, // .dir <symbol>
    LIT, // .dir <literal>
    SYM_LIT, // .dir <symbol>, <literal>
    SYM_LIST, // .dir <symbol list>
    SYM_LIT_LIST // .dir <symbol/literal list>
};

enum Directive
{
    GLOBAL,
    EXTERN,
    SECTION,
    WORD,
    SKIP,
    EQU,
    END
};

struct DirInfo
{
    DirInfo(Directive dir, DirArgFormat argFormat, bool labelsAllowed, bool sectionRequired) :
        dir(dir), argFormat(argFormat), labelsAllowed(labelsAllowed), sectionRequired(sectionRequired)
    {}

    Directive dir;
    DirArgFormat argFormat;
    bool labelsAllowed;
    bool sectionRequired;
};

const std::unordered_map<std::string, DirInfo> DIRECTIVES({
    { "global",  { GLOBAL, SYM_LIST, false, false } },
    { "extern",  { EXTERN, SYM_LIST, false, false } },
    { "section", { SECTION, SYM, false, false } },
    { "word",    { WORD, SYM_LIT_LIST, true, true } },
    { "skip",    { SKIP, LIT, true, true } },
    { "equ",     { EQU, SYM_LIT, false, false } },
    { "end",     { END, NONE, false, false } }
});

#endif