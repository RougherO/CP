#pragma once
/// @file
/// @brief cp::debug::unreachable: marks a point the program never reaches (`std::unreachable` from C++23).
#include "cp/config.hpp"
#if defined(__cpp_lib_unreachable)  // cp:since c++23
#  include <utility>
#endif

namespace cp::debug {

#if defined(__cpp_lib_unreachable)  // cp:since c++23
using std::unreachable;
#else
/// @brief Marks a point that is never reached; reaching it is undefined behaviour.
[[noreturn]] inline void unreachable() { __builtin_unreachable(); }
#endif

}  // namespace cp::debug
