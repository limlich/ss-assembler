#ifndef DIRECTIVE_H
#define DIRECTIVE_H

#include <variant>
#include <string>

#include "types.hpp"

typedef std::variant<std::string, ushort> dir_arg_type;

#endif