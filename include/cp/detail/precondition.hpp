#pragma once
/// @file
/// @brief Precondition reporting for backports (temporary, until `cp::check`).
#include "cp/compat/source_location.hpp"
#include <cstdio>
#include <cstdlib>
#include <string_view>

namespace cp::detail {

/// @brief If `!ok`: flush stdout, print `file:line: function: precondition failed: what`, abort.
/// During constant evaluation a violation is a compile error. Reporting is a no-op when CP_CHECKS is 0, but `ok` is
/// still evaluated: pass cheap, pure conditions.
constexpr void precondition([[maybe_unused]] bool ok, [[maybe_unused]] std::string_view what,
                            [[maybe_unused]] source_location loc = source_location::current()) noexcept {
#if CP_CHECKS
    if (CP_UNLIKELY(!ok)) {
        std::fflush(stdout);
        std::fprintf(stderr, "%s:%u: %s: precondition failed: %.*s\n", loc.file_name(),
                     static_cast<unsigned>(loc.line()), loc.function_name(), static_cast<int>(what.size()),
                     what.data());
        std::abort();
    }
#endif
}

}  // namespace cp::detail
