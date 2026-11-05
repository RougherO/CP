#pragma once
/// @file
/// @brief `__cpp_lib_*` feature-test macros; included first by every compat header.
#include "cp/config.hpp"
#if __has_include(<version>)
#  include <version>
#endif
