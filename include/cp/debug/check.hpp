#pragma once
/// @file
/// @brief CP_CHECK and cp::debug::check: stop the program with the caller's line when a condition is false, under
/// `CP_CHECKS`; nothing at all otherwise.
/// @par Caveats
/// - `CP_CHECK(cond)` is the one cp macro for user code: without `CP_CHECKS`, `cond` is not evaluated (it still has to
///   compile). On failure it reports `file:line: function: check failed: cond`.
/// - cp::debug::check is the function behind it, for a computed message. As a function it evaluates its arguments
///   either way; the optimiser deletes conditions without side effects.
/// - A failure flushes cp::io::out, writes the report to cp::io::err, and aborts. In a constant expression it is a
///   compile error. The library's own checks use cp::detail::precondition instead, which writes to standard error.
#include "cp/config.hpp"
#include "cp/debug/source_location.hpp"
#include "cp/detail/precondition.hpp"
#include "cp/io/streams.hpp"
#include <cstdlib>
#include <string_view>

namespace cp::detail {

/// @brief Flushes cp::io::out, writes failure_text to cp::io::err, aborts: how contestant checks fail.
[[noreturn]] CP_NOINLINE inline void report_failure(std::string_view msg, debug::source_location loc) {
    io::out.flush();
    io::err.write(failure_text(msg, loc));
    std::abort();
}

}  // namespace cp::detail

/// @namespace cp::debug
/// @brief Debugging aids: checks, dbg, source locations, unreachable code.
namespace cp::debug {

/// @brief Under `CP_CHECKS`, stops the program with `msg` and the caller's location if `cond` is false.
constexpr void check([[maybe_unused]] bool cond, [[maybe_unused]] std::string_view msg,
                     [[maybe_unused]] source_location loc = source_location::current()) {
#if CP_CHECKS
    if (CP_UNLIKELY(!cond)) detail::report_failure(msg, loc);
#endif
}

}  // namespace cp::debug

/// @def CP_CHECK
/// @brief `CP_CHECK(cond)`: under `CP_CHECKS`, stops the program with `check failed: cond` and the line of the call
/// if `cond` is false. Otherwise `cond` is only compiled, never evaluated.
#if CP_CHECKS
#  define CP_CHECK(...) ::cp::debug::check(static_cast<bool>(__VA_ARGS__), "check failed: " #__VA_ARGS__)
#else
#  define CP_CHECK(...) static_cast<void>(sizeof(static_cast<bool>(__VA_ARGS__)))
#endif
