#ifndef TYPES_H
#define TYPES_H

#include <cstdint>
#include <variant>
#include <string>

typedef uint8_t ubyte;
typedef uint16_t ushort;
typedef uint32_t uint;

typedef std::variant<std::string, ushort> string_ushort_variant;


#endif