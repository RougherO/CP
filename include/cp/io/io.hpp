#pragma once
/// @file
/// @brief The standard streams cp::io::in, cp::io::out and cp::io::err, and the free functions cp::io::read,
/// cp::io::readln, cp::io::write, cp::io::writeln and cp::io::getline, the formatters, and cp::io::dump.
/// @code
/// int n;
/// cp::io::read(n);
/// std::vector<long long> a(n);
/// cp::io::read(a);
/// auto [h, w] = *cp::io::read<int, int>();
/// while (cp::io::read(x)) { … }
/// cp::io::writeln(n, a);
/// @endcode
/// @par Caveats
/// - cp::io::in is tied to cp::io::out: output is flushed before the program waits for input, so interactive problems
///   need no manual flush. cp::io::out is flushed at exit (static destruction); after `std::quick_exit` or a crash,
///   call `cp::io::out.flush()` first.
/// - Unlike `std::cin`, the tie costs nothing measurable: it flushes only before waiting for more input from a pipe
///   (once per 64 KiB), never for a file, not before every read. `cp::io::in.tie(nullptr)` unties, but gains nothing
///   and would hang an interactive problem.
/// - cp::io::err writes through on every call, so diagnostics survive crashes.
/// - Don't mix cp::io::out with `printf`/`std::cout` in one program: each has its own buffer, so the output interleaves
///   in the wrong order.
/// - Every read returns a cp::io::read_result: `if (!read(a, b))`, `while (read(x))`, `int n = *read<int>();`. With
///   `CP_CHECKS`, a bad token (`invalid`, `out_of_range`) is reported at the line of the cp::io::read call; running out
///   of input is returned, not reported.
/// - `using namespace cp;` at global scope makes `write(fd, buf, n)` call cp::io::write; qualify POSIX `::write`.
#include "cp/config.hpp"
#include "cp/traits/traits.hpp"
#include "cp/debug/source_location.hpp"
#include "cp/debug/check.hpp"
#include "cp/detail/located_arg.hpp"
#include "cp/io/dump.hpp"  // IWYU pragma: export (cp::io::dump is part of the io module)
#include "cp/io/streams.hpp"
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace cp::detail {

/// @brief What cp::io::read does with its first argument: a formatter governs the arguments after it; anything else is
/// read. A temporary is rejected at the caller's line.
struct read_op {
    /// @brief Applies or reads `x` on `r`; `governing` is what the next arguments are read with.
    template <class T>
    void operator()(io::reader& r, detail::input_settings& local, detail::input_settings const*& governing,
                    T&& x) const {
        static_assert(read_targets<is_variable, T&&>::value, "cp::io::read needs variables, not temporaries");
        static_assert(read_targets<is_mutable, T&&>::value, "cp::io::read cannot read into const");
        if constexpr (is_formatter_v<traits::remove_cvref_t<T>>) {
            local = stream_access::start(r);
            x.apply(local);
            governing = &local;
        } else {
            stream_access::read(r, *governing, *governing, x);
        }
    }
};

/// @brief What cp::io::read<T, Ts...>() returns a value of: `T`, or `std::tuple<T, Ts...>` for several types.
template <class T, class... Ts>
using read_value_t = std::conditional_t<sizeof...(Ts) == 0, T, std::tuple<T, Ts...>>;

/// @brief Under `CP_CHECKS`, reports a bad token (an `invalid` or `out_of_range` result `r` of cp::io::in) at `loc`,
/// with its input line. End of input is not reported: it is how read loops end.
inline void report_bad_input([[maybe_unused]] io::read_result<> const& r, [[maybe_unused]] debug::source_location loc) {
#if CP_CHECKS
    if (r || r.error() == io::read_error::end_of_input) return;
    constexpr std::size_t shown = 40;  // characters of the bad token quoted in the report
    report_failure(std::string("cp::io::read: input line ")
                       .append(std::to_string(io::in.line()))
                       .append(r.error() == io::read_error::invalid ? ": invalid token \"" : ": out-of-range number \"")
                       .append(io::in.token().substr(0, shown))
                       .append("\""),
                   loc);
#endif
}

}  // namespace cp::detail

namespace cp::io {

#if CP_CHECKS
/// @brief Reads each argument from cp::io::in, like `std::cin >> a >> b`: success, or the read_error that stopped it.
/// A bad token is reported at the caller's line.
template <class... Rest>
auto read(detail::located_arg<detail::read_op, reader&, detail::input_settings&, detail::input_settings const*&> first,
          Rest&&... rest) -> read_result<> {
    static_assert((detail::read_targets<detail::is_variable, Rest>::value && ...),
                  "cp::io::read needs variables, not temporaries");
    static_assert((detail::read_targets<detail::is_mutable, Rest>::value && ...),
                  "cp::io::read cannot read into const");
    detail::input_settings local;
    detail::input_settings const* governing = &detail::stream_access::start(in);
    first(in, local, governing);
    auto r = detail::stream_access::read(in, detail::stream_access::start(in), *governing, rest...);
    detail::report_bad_input(r, first.location());
    return r;
}
/// @brief read(first, rest...), then drops the rest of the line and its `\n`.
template <class... Rest>
auto readln(
    detail::located_arg<detail::read_op, reader&, detail::input_settings&, detail::input_settings const*&> first,
    Rest&&... rest) -> read_result<> {
    auto r = read(first, std::forward<Rest>(rest)...);
    if (r) detail::stream_access::skip_line(in);
    return r;
}
#else
/// @brief Reads each argument from cp::io::in, like `std::cin >> a >> b`: success, or the read_error that stopped it.
template <class... Args>
auto read(Args&&... args) -> read_result<> {
    return in.read(std::forward<Args>(args)...);
}
/// @brief read(args...), then drops the rest of the line and its `\n`.
template <class... Args>
auto readln(Args&&... args) -> read_result<> {
    return in.readln(std::forward<Args>(args)...);
}
#endif
/// @brief Skips to the next line of cp::io::in.
inline auto readln() -> read_result<> { return in.readln(); }

/// @brief Reads a `T` from cp::io::in (several types: a `std::tuple`, for structured bindings), or the read_error that
/// prevented it: `int n = *read<int>();`, `while (auto x = read<int>()) ...`. Under `CP_CHECKS`, a bad token is
/// reported at the caller's line.
template <class T, class... Ts>
[[nodiscard]] auto read([[maybe_unused]] debug::source_location loc = debug::source_location::current())
    -> read_result<detail::read_value_t<T, Ts...>> {
    detail::read_value_t<T, Ts...> value{};
    auto r = in.read(value);
    detail::report_bad_input(r, loc);
    if (!r) return util::failure(r.error());
    return value;
}
/// @brief read<T, Ts...>() with the formatter `f`: `auto h = *read<int>(base(16));`.
template <class T, class... Ts, class F, std::enable_if_t<detail::is_formatter_v<F>, int> = 0>
[[nodiscard]] auto read(F const& f, [[maybe_unused]] debug::source_location loc = debug::source_location::current())
    -> read_result<detail::read_value_t<T, Ts...>> {
    detail::read_value_t<T, Ts...> value{};
    auto r = in.read(f, value);
    detail::report_bad_input(r, loc);
    if (!r) return util::failure(r.error());
    return value;
}

/// @brief Writes each argument to cp::io::out, separated by the argument separator; a formatter argument governs the
/// arguments after it.
template <class... Args>
auto write(Args const&... args) -> writer& {
    return out.write(args...);
}
/// @brief write(args...), then `\n`.
template <class... Args>
auto writeln(Args const&... args) -> writer& {
    return out.writeln(args...);
}

/// @brief Reads a line of cp::io::in into `line`, like `std::getline` (a trailing `\r` is dropped).
inline auto getline(std::string& line) -> read_result<> { return in.getline(line); }

}  // namespace cp::io
