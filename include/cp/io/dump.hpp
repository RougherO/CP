#pragma once
/// @file
/// @brief cp::io::dump: values pretty-printed with the file and line, to cp::io::err, under `CP_CHECKS`.
/// @code
/// cp::io::dump(n, v, edges);   // [main.cpp:12] 3 | [1, 2, 3] | [{u: 1, v: 2}]
/// cp::io::dump();              // [main.cpp:13]: the program got here
/// @endcode
/// @par Caveats
/// - Values start from cp::io::dump_style instead of the defaults; formatter arguments apply on top of it, as in any
///   call: `dump(io::precision(3), d)`.
/// - cp::io::out is flushed first, so on a terminal the line appears after the output written before it.
/// - Without `CP_CHECKS` the call writes nothing, but its arguments are still evaluated (it is a function): keep
///   side effects out of them.
#include "cp/config.hpp"
#include "cp/debug/source_location.hpp"
#include "cp/detail/located_arg.hpp"
#include "cp/detail/precondition.hpp"
#include "cp/io/options.hpp"
#include "cp/io/streams.hpp"
#include "cp/traits/traits.hpp"
#include <charconv>

namespace cp::io {

/// @brief How cp::io::dump writes values: ` | ` between them, `[a, b]` ranges, `{a, b}` records with field names,
/// quoted text, `true`/`false`, the shortest exact floats, and lines wrapped at 80 characters.
inline constexpr auto dump_style = arg_sep(" | ") | range_sep(", ") | range_bracket("[", "]") | field_sep(", ") |
                                   field_bracket("{", "}") | show_bool_words() |
                                   float_format(std::chars_format::general) | precision(-1) | show_quotes() |
                                   show_field_names() | wrap(80);

}  // namespace cp::io

namespace cp::detail {

/// @brief What cp::io::dump does with its first argument: a formatter governs the values after it; a value is written.
struct dump_op {
    /// @brief Applies `x` on top of `style`, or writes it to `w`.
    template <class T>
    void operator()(io::writer& w, detail::output_settings const& style, detail::output_settings& local,
                    detail::output_settings const*& governing, bool& first, T&& x) const {
        if constexpr (is_formatter_v<traits::remove_cvref_t<T>>) {
            local = style;
            x.apply(local);
            governing = &local;
        } else {
            stream_access::write_item(w, first, *governing, x);
        }
    }
};

/// @brief The settings of cp::io::dump_style.
inline auto dump_settings() noexcept -> detail::output_settings {
    detail::output_settings s;
    io::dump_style.apply(s);
    return s;
}

}  // namespace cp::detail

namespace cp::io {

/// @brief Under `CP_CHECKS`, writes `[%file:line]` and `\n` to cp::io::err: the program got here.
inline void dump([[maybe_unused]] debug::source_location loc = debug::source_location::current()) {
#if CP_CHECKS
    out.flush();
    err.put("[" + detail::file_line(loc) + "]\n");
    err.flush();
#endif
}

/// @brief Under `CP_CHECKS`, writes `[%file:line]`, the values in cp::io::dump_style (formatter arguments on top), and
/// `\n` to cp::io::err. Otherwise nothing, though the arguments are evaluated.
template <class... Rest>
void dump(detail::located_arg<detail::dump_op, writer&, detail::output_settings const&, detail::output_settings&,
                              detail::output_settings const*&, bool&>
              first,
          [[maybe_unused]] Rest const&... rest) {
#if CP_CHECKS
    out.flush();
    err.put("[" + detail::file_line(first.location()) + "] ");
    detail::output_settings const style = detail::dump_settings();
    detail::output_settings local;
    detail::output_settings const* governing = &style;
    bool first_value                         = true;
    first(err, style, local, governing, first_value);
    detail::stream_access::write(err, style, *governing, first_value, rest...);
    err.put('\n');
    err.flush();
#else
    static_cast<void>(first);
#endif
}

}  // namespace cp::io
