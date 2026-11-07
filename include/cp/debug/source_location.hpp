#pragma once
/// @file
/// @brief Backport of C++20 `std::source_location`.
/// @par Caveats
/// - GCC, C++17: column() is always 0 (no `__builtin_COLUMN`). Use C++20 for columns. cp's reports print lines only.
/// - file_name()/function_name() return `char const*` like std (null-terminated literals).
/// - function_name() format is implementation-defined (`main` vs `int main()`); display only.
#include "cp/config.hpp"
#if defined(__cpp_lib_source_location)  // cp:since c++20
#  include <source_location>
#else
#  include <cstdint>
#endif

namespace cp::debug {

#if defined(__cpp_lib_source_location)  // cp:since c++20
using std::source_location;
#else
/// @brief File, line, column and function of a point in the source.
class source_location {
public:
    /// @brief Location of the call site. Never pass the arguments explicitly.
    static constexpr auto current(std::uint_least32_t line = __builtin_LINE(),
#  if CP_HAS_BUILTIN_COLUMN
                                  std::uint_least32_t column = __builtin_COLUMN(),
#  else
                                  std::uint_least32_t column = 0,
#  endif
                                  char const* file     = __builtin_FILE(),
                                  char const* function = __builtin_FUNCTION()) noexcept {
        source_location loc;
        loc.m_line     = line;
        loc.m_column   = column;
        loc.m_file     = file;
        loc.m_function = function;
        return loc;
    }

    /// @brief Empty location (line 0).
    constexpr source_location() noexcept = default;

    /// @brief 1-based line number.
    constexpr auto line() const noexcept { return m_line; }
    /// @brief 1-based column, or 0 if unknown.
    constexpr auto column() const noexcept { return m_column; }
    /// @brief File name as passed to the compiler.
    constexpr auto file_name() const noexcept { return m_file; }
    /// @brief Enclosing function name.
    constexpr auto function_name() const noexcept { return m_function; }

private:
    std::uint_least32_t m_line   = 0;
    std::uint_least32_t m_column = 0;
    char const* m_file           = "";
    char const* m_function       = "";
};
#endif

}  // namespace cp::debug
