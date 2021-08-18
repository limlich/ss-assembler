#ifndef REGISTER_H
#define REGISTER_H

#include <unordered_map>
#include <string>

#include "types.hpp"


enum Register: ubyte
{
    SP_REGISTER = 6u,
    PC_REGISTER = 7u,
    PSW_REGISTER = 8u,
    NUM_REGISTERS
};

const std::unordered_map<std::string, ubyte> REGISTERS({
    { "r0", 0u },
    { "r1", 1u },
    { "r2", 2u },
    { "r3", 3u },
    { "r4", 4u },
    { "r5", 5u },
    { "r6", 6u },
    { "r7", 7u },
    { "sp", SP_REGISTER },
    { "pc", PC_REGISTER },
    { "psw", PSW_REGISTER }
});

enum MMapRegister: ushort
{
    TERM_OUT = 0xFF00,
    TERM_IN = 0xFF02,
    TIM_CFG = 0xFF10
};

enum RegIndUpdateType: ubyte
{
    REGIND_NONE = 0x00u,
    REGIND_PRE_DEC = 0x01u,
    REGIND_PRE_INC = 0x02u,
    REGIND_POST_DEC = 0x03u,
    REGIND_POST_INC = 0x04u
};

#endif