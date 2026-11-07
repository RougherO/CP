#pragma once
/// @file
/// @brief Formatters: the settings of cp's reader and writer, passed per call. `io::precision(3)`, `io::base(16)`,
/// `io::range_sep(", ")`, … are formatters; `f | g` combines them, `io::defaults()` is the identity (back to the
/// defaults).
/// @code
/// io::writeln(io::precision(3), x, y, io::defaults(), z);   // x, y with 3 digits; z with the defaults
/// io::writeln("ans:", io::range_sep(",")(v));           // only v is decorated
/// io::read(io::base(16), h);                            // h read in hexadecimal
/// @endcode
/// @par Caveats
/// - Streams keep no settings: every call starts from the defaults. A formatter argument governs the arguments after
///   it, up to the next formatter argument, which replaces it (layer them with `|`). `f(values...)` decorates only
///   those values, on top of the formatter governing them.
/// - `f(values...)` holds references to the values: use it within the full expression, as with
///   `std::forward_as_tuple`. A formatter keeps the strings it is given as `std::string_view`: keep one only while
///   they live (literals always do; `io::range_sep(std::string(…))` dangles after its statement).
/// - `signed char` and `unsigned char` (so `std::int8_t`, `std::uint8_t`) are integers, not characters: they read
///   and write as numbers. Only `char` is a character. (iostreams treat all three as characters.)
#include "cp/config.hpp"
#include <array>
#include <charconv>
#include <cstddef>
#include <string_view>
#include <tuple>
#include <utility>

namespace cp::detail {

/// @brief Separators of nested ranges when no `range_sep` is given: `" "` inside a range of numbers, `"\n"` above.
inline constexpr std::array<std::string_view, 2> default_range_sep{" ", "\n"};

/// @brief Everything a formatter can set for writing; the members hold the defaults.
struct output_settings {
    int base                          = 10;   ///< Integer base, 2–36; digits above 9 are lower-case letters.
    int width                         = 0;    ///< Minimum integer width, padded on the left.
    char fill                         = ' ';  ///< Padding character; `'0'` pads after the sign, as `std::format` does.
    bool show_sign                    = false;                     ///< `+` before non-negative integers.
    std::chars_format float_format    = std::chars_format::fixed;  ///< As for `std::to_chars`.
    int precision                     = 9;      ///< Digits after the point; negative: shortest round-trip.
    bool bool_alpha                   = false;  ///< `true`/`false` instead of `1`/`0`.
    std::string_view arg_sep          = " ";    ///< Between the arguments of one call.
    std::string_view const* range_sep = detail::default_range_sep.data();  ///< Per nesting level, innermost first.
    std::size_t range_sep_count = detail::default_range_sep.size();  ///< Levels in range_sep; deeper reuse the last.
    std::string_view field_sep  = " ";         ///< Between the fields of tuple-likes and plain structs.
    std::string_view range_open, range_close;  ///< Around every range.
    std::string_view field_open, field_close;  ///< Around every tuple-like and plain struct.
    bool quote       = false;                  ///< Strings in `"…"`, chars in `'…'`, escaped.
    bool field_names = false;                  ///< Plain structs as `name: value` where names are known.
    int wrap         = 0;  ///< Break ranges and records wider than this, one element per line; 0: never.
};

/// @brief Everything a formatter can set for reading; the members hold the defaults.
struct input_settings {
    int base                       = 10;                          ///< Integer base, 2–36, either case.
    std::chars_format float_format = std::chars_format::general;  ///< Accepted forms, as for `std::from_chars`.
    bool skip_ws                   = true;  ///< Reading a `char` skips whitespace first, as `std::cin >> c` does.
};

}  // namespace cp::detail

namespace cp::io {

template <class F, class... Ts>
struct formatted;

/// @brief A formatter: settings applied in order (the right one wins on the same field). Call it on values to decorate
/// them, pass it as an argument to govern the arguments after it, combine it with `|`. Not named `formatter`: that
/// would clash with `std::formatter` when both namespaces are open.
template <class... Layers>
class formatting {
public:
    /// @brief The identity: no settings.
    constexpr formatting() = default;
    /// @brief The formatter applying `layers` in order.
    constexpr explicit formatting(std::tuple<Layers...> layers) noexcept : m_layers(std::move(layers)) { }

    /// @brief `values...` decorated by this formatter, for one read or write.
    template <class... Ts>
    constexpr auto operator()(Ts&&... values) const noexcept -> formatted<formatting, Ts&&...> {
        return {*this, std::forward_as_tuple(std::forward<Ts>(values)...)};
    }

    /// @brief Applies the settings to `s` (writing). Pointers it stores point into this formatter.
    constexpr void apply(detail::output_settings& s) const noexcept {
        static_assert((Layers::writes && ...), "cp::io: this formatter has a setting that only applies to reading");
        std::apply([&s](auto const&... l) { (l.write(s), ...); }, m_layers);
    }
    /// @brief Applies the settings to `s` (reading).
    constexpr void apply(detail::input_settings& s) const noexcept {
        static_assert((Layers::reads && ...), "cp::io: this formatter has a setting that only applies to writing");
        std::apply([&s](auto const&... l) { (l.read(s), ...); }, m_layers);
    }

    /// @brief The settings, in order.
    constexpr auto layers() const noexcept -> std::tuple<Layers...> const& { return m_layers; }

private:
    std::tuple<Layers...> m_layers;
};

/// @brief Values decorated by a formatter (what `f(values...)` returns): `Ts` are references.
template <class F, class... Ts>
struct formatted {
    F fmt;                     ///< The formatter.
    std::tuple<Ts...> values;  ///< The values, by reference.
};

/// @brief `a`, then `b`.
template <class... A, class... B>
constexpr auto operator|(formatting<A...> const& a, formatting<B...> const& b) noexcept -> formatting<A..., B...> {
    return formatting<A..., B...>(std::tuple_cat(a.layers(), b.layers()));
}

}  // namespace cp::io

namespace cp::detail {

/// @brief True for cp::io::formatting types (formatters).
template <class T>
inline constexpr bool is_formatter_v = false;
template <class... L>
inline constexpr bool is_formatter_v<io::formatting<L...>> = true;

/// @brief True for cp::io::formatted types.
template <class T>
inline constexpr bool is_formatted_v = false;
template <class F, class... Ts>
inline constexpr bool is_formatted_v<io::formatted<F, Ts...>> = true;

/// @brief A setting of one field, both directions.
template <class T, T detail::output_settings::* Out, T detail::input_settings::* In>
struct both_layer {
    static constexpr bool writes = true;  ///< Applies to writing.
    static constexpr bool reads  = true;  ///< Applies to reading.
    T value;                              ///< The field's value.
    /// @brief Sets the field for writing.
    constexpr void write(detail::output_settings& s) const noexcept { s.*Out = value; }
    /// @brief Sets the field for reading.
    constexpr void read(detail::input_settings& s) const noexcept { s.*In = value; }
};

/// @brief A setting of one field, writing only.
template <class T, T detail::output_settings::* Out>
struct write_layer {
    T value;  ///< The field's value.
    /// @brief Sets the field.
    constexpr void write(detail::output_settings& s) const noexcept { s.*Out = value; }
    static constexpr bool writes = true;   ///< Applies to writing.
    static constexpr bool reads  = false;  ///< Applies to reading.
};

/// @brief A setting of one field, reading only.
template <class T, T detail::input_settings::* In>
struct read_layer {
    T value;                               ///< The field's value.
    static constexpr bool writes = false;  ///< Applies to writing.
    static constexpr bool reads  = true;   ///< Applies to reading.
    /// @brief Sets the field.
    constexpr void read(detail::input_settings& s) const noexcept { s.*In = value; }
};

/// @brief Two fields set together (brackets), writing only.
template <std::string_view detail::output_settings::* Open, std::string_view detail::output_settings::* Close>
struct bracket_layer {
    std::string_view open, close;  ///< The brackets.
    /// @brief Sets both fields.
    constexpr void write(detail::output_settings& s) const noexcept {
        s.*Open  = open;
        s.*Close = close;
    }
    static constexpr bool writes = true;   ///< Applies to writing.
    static constexpr bool reads  = false;  ///< Applies to reading.
};

/// @brief The range separators, innermost level first.
template <std::size_t N>
struct range_sep_layer {
    std::array<std::string_view, N> seps;  ///< One per level.
    /// @brief Points the settings at `seps`.
    constexpr void write(detail::output_settings& s) const noexcept {
        s.range_sep       = seps.data();
        s.range_sep_count = N;
    }
    static constexpr bool writes = true;   ///< Applies to writing.
    static constexpr bool reads  = false;  ///< Applies to reading.
};

/// @brief The formatter holding one layer.
template <class L>
constexpr auto one_layer(L l) noexcept -> io::formatting<L> {
    return io::formatting<L>(std::tuple<L>(l));
}

}  // namespace cp::detail

namespace cp::io {

/// @brief Integer base, 2–36, for reading and writing (default 10).
constexpr auto base(int b) noexcept {
    return detail::one_layer(detail::both_layer<int, &detail::output_settings::base, &detail::input_settings::base>{b});
}
/// @brief Floating-point form, `std::chars_format` (default: `fixed` writing, `general` reading).
constexpr auto float_format(std::chars_format f) noexcept {
    return detail::one_layer(detail::both_layer<std::chars_format, &detail::output_settings::float_format,
                                                &detail::input_settings::float_format>{f});
}
/// @brief Digits after the point when writing floating point (default 9); negative: shortest round-trip.
constexpr auto precision(int p) noexcept {
    return detail::one_layer(detail::write_layer<int, &detail::output_settings::precision>{p});
}
/// @brief Minimum width of written integers, padded on the left (default 0).
constexpr auto width(int w) noexcept {
    return detail::one_layer(detail::write_layer<int, &detail::output_settings::width>{w});
}
/// @brief Padding character for width (default `' '`); `'0'` pads after the sign.
constexpr auto fill(char c) noexcept {
    return detail::one_layer(detail::write_layer<char, &detail::output_settings::fill>{c});
}
/// @brief `+` before non-negative integers.
constexpr auto show_sign(bool on = true) noexcept {
    return detail::one_layer(detail::write_layer<bool, &detail::output_settings::show_sign>{on});
}
/// @brief `true`/`false` instead of `1`/`0`.
constexpr auto bool_alpha(bool on = true) noexcept {
    return detail::one_layer(detail::write_layer<bool, &detail::output_settings::bool_alpha>{on});
}
/// @brief Whether reading a `char` skips whitespace (default true).
constexpr auto skip_ws(bool on) noexcept {
    return detail::one_layer(detail::read_layer<bool, &detail::input_settings::skip_ws>{on});
}
/// @brief Separator between the arguments of one call (default `" "`).
constexpr auto arg_sep(std::string_view s) noexcept {
    return detail::one_layer(detail::write_layer<std::string_view, &detail::output_settings::arg_sep>{s});
}
/// @brief Separator between the fields of tuple-likes and plain structs (default `" "`).
constexpr auto field_sep(std::string_view s) noexcept {
    return detail::one_layer(detail::write_layer<std::string_view, &detail::output_settings::field_sep>{s});
}
/// @brief Separators inside ranges, innermost level first; deeper levels reuse the last (default `" "`, `"\n"`).
template <class... S>
constexpr auto range_sep(S const&... seps) noexcept {
    static_assert(sizeof...(S) > 0, "cp::io::range_sep needs at least one separator");
    return detail::one_layer(detail::range_sep_layer<sizeof...(S)>{{std::string_view(seps)...}});
}
/// @brief Brackets around every range (default none).
constexpr auto range_bracket(std::string_view open, std::string_view close) noexcept {
    return detail::one_layer(
        detail::bracket_layer<&detail::output_settings::range_open, &detail::output_settings::range_close>{open,
                                                                                                           close});
}
/// @brief Brackets around every tuple-like and plain struct (default none).
constexpr auto field_bracket(std::string_view open, std::string_view close) noexcept {
    return detail::one_layer(
        detail::bracket_layer<&detail::output_settings::field_open, &detail::output_settings::field_close>{open,
                                                                                                           close});
}
/// @brief Strings in `"…"` and chars in `'…'`, with quotes, `\\`, newlines and tabs escaped.
constexpr auto quote(bool on = true) noexcept {
    return detail::one_layer(detail::write_layer<bool, &detail::output_settings::quote>{on});
}
/// @brief Plain structs as `name: value` where field names are known (C++20 and later).
constexpr auto field_names(bool on = true) noexcept {
    return detail::one_layer(detail::write_layer<bool, &detail::output_settings::field_names>{on});
}
/// @brief Ranges and records wider than `w` characters break one element per line; 0: never (default).
constexpr auto wrap(int w) noexcept {
    return detail::one_layer(detail::write_layer<int, &detail::output_settings::wrap>{w});
}

/// @brief The identity: no settings, so the arguments after it get the defaults.
constexpr auto defaults() noexcept -> formatting<> { return {}; }

}  // namespace cp::io
