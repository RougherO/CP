#pragma once
/// @file
/// @brief cp::detail::precondition: the library's own checks, under `CP_CHECKS`. A failure writes
/// `file:line: function: message` straight to standard error and aborts, so every module can check its
/// preconditions, I/O included. Contestant checks (CP_CHECK) and cp::io::dump report through cp::io::err instead.
#include "cp/config.hpp"
#include "cp/debug/source_location.hpp"
#include "cp/sys/output_sink.hpp"
#include <cstdlib>
#include <string>
#include <string_view>

namespace cp::detail {

/// @brief `file:line` for `loc`, the file without its directory.
inline auto file_line(debug::source_location loc) -> std::string {
    std::string_view file = loc.file_name();
    file.remove_prefix(file.find_last_of("/\\") + 1);  // npos + 1 == 0: no directory
    return std::string(file).append(":").append(std::to_string(loc.line()));
}

/// @brief `file:line: function: msg` and a newline: the text of every cp failure report.
inline auto failure_text(std::string_view msg, debug::source_location loc) -> std::string {
    std::string text;
    if (loc.line() != 0) text.append(file_line(loc)).append(": ").append(loc.function_name()).append(": ");
    return text.append(msg).append("\n");
}

/// @brief Writes failure_text to standard error and aborts.
[[noreturn]] CP_NOINLINE inline void precondition_failed(std::string_view msg, debug::source_location loc) {
    std::string const text = failure_text(msg, loc);
    output_sink(2).write_all(text.data(), text.size());
    std::abort();
}

/// @brief Under `CP_CHECKS`, stops the program with `msg` and `loc` if `cond` is false. In a constant expression a
/// failure is a compile error.
constexpr void precondition([[maybe_unused]] bool cond, [[maybe_unused]] std::string_view msg,
                            [[maybe_unused]] debug::source_location loc = debug::source_location::current()) {
#if CP_CHECKS
    if (CP_UNLIKELY(!cond)) precondition_failed(msg, loc);
#endif
}

}  // namespace cp::detail
