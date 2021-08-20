#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include <unordered_map>

#include "types.hpp"

typedef ubyte addr_mode_type;

enum AddrMode: addr_mode_type
{
    AM_IMMED = 0b0000u,
    AM_REGDIR = 0b0001u,
    AM_REGDIR_OFFSET = 0b0101u,
    AM_REGIND = 0b0010u,
    AM_REGIND_OFFSET = 0b0011u,
    AM_MEMDIR = 0b0100u
};

enum AddrModeMask: addr_mode_type
{
    IMMED = 1u,
    REGDIR = 1u << 1,
    REGDIR_OFFSET = 1u << 2,
    REGIND = 1u << 3,
    REGIND_OFFSET = 1u << 4,
    MEMDIR = 1u << 5
};

const addr_mode_type ANY_ADDR_MODE =
    IMMED |
    REGDIR |
    REGDIR_OFFSET |
    REGIND |
    REGIND_OFFSET |
    MEMDIR;

struct InstrInfo
{
    InstrInfo(ubyte opCode, bool jmpSyntax, ubyte numArgs, addr_mode_type arg1AddrModes, addr_mode_type arg2AddrModes) :
        opCode(opCode), jmpSyntax(jmpSyntax), numArgs(numArgs), argAddrModes{arg1AddrModes, arg2AddrModes}
    {}

    ubyte opCode;
    bool jmpSyntax; // uses jmp operand syntax?
    ubyte numArgs;
    addr_mode_type argAddrModes[2];
};

struct InstrArg
{
    InstrArg() : jmpSyntax(false) {}

    bool jmpSyntax;
    addr_mode_type addrMode;
    string_ushort_variant val; // value
    string_ushort_variant off; // offset
};

const std::unordered_map<std::string, InstrInfo> INSTRUCTIONS({
    { "halt", { 0x00u, false, 0, 0, 0 } },
    { "int",  { 0x10u, false, 1, REGDIR, 0 } },
    { "iret", { 0x20u, false, 0, 0, 0 } },
    { "call", { 0x30u,  true, 1, ANY_ADDR_MODE, 0 } },
    { "ret",  { 0x40u, false, 0, 0, 0 } },
    { "jmp",  { 0x50u,  true, 1, ANY_ADDR_MODE, 0 } },
    { "jeq",  { 0x51u,  true, 1, ANY_ADDR_MODE, 0 } },
    { "jne",  { 0x52u,  true, 1, ANY_ADDR_MODE, 0 } },
    { "jgt",  { 0x53u,  true, 1, ANY_ADDR_MODE, 0 } },
    { "xchg", { 0x60u, false, 2, REGDIR, REGDIR } },
    { "add",  { 0x70u, false, 2, REGDIR, REGDIR } },
    { "sub",  { 0x71u, false, 2, REGDIR, REGDIR } },
    { "mul",  { 0x72u, false, 2, REGDIR, REGDIR } },
    { "div",  { 0x73u, false, 2, REGDIR, REGDIR } },
    { "cmp",  { 0x74u, false, 2, REGDIR, REGDIR } },
    { "not",  { 0x80u, false, 1, REGDIR, 0 } },
    { "and",  { 0x81u, false, 2, REGDIR, REGDIR } },
    { "or",   { 0x82u, false, 2, REGDIR, REGDIR } },
    { "xor",  { 0x83u, false, 2, REGDIR, REGDIR } },
    { "test", { 0x84u, false, 2, REGDIR, REGDIR } },
    { "shl",  { 0x90u, false, 2, REGDIR, REGDIR } },
    { "shr",  { 0x91u, false, 2, REGDIR, REGDIR } },
    { "ldr",  { 0xA0u, false, 2, REGDIR, ANY_ADDR_MODE } },
    { "str",  { 0xB0u, false, 2, REGDIR, ANY_ADDR_MODE&~IMMED } },
    { "push", { 0xB0u, false, 1, REGDIR, 0 } },
    { "pop",  { 0xA0u, false, 1, REGDIR, 0 } }
});

#endif