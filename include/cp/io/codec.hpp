#pragma once
/// @file
/// @brief How cp's reader and writer classify a type (cp::detail::io_kind_of): formatters, formatted values, a user
/// hook (`cp_read`/`cp_write` found by argument-dependent lookup), the exact built-ins, numbers, ranges, tuple-likes,
/// then plain structs.
#include "cp/config.hpp"
#include "cp/detail/range.hpp"
#include "cp/io/aggregate.hpp"
#include "cp/io/options.hpp"
#include "cp/traits/traits.hpp"
#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace cp::io {

class reader;
class writer;

}  // namespace cp::io

namespace cp::detail {

/// @brief What cp's free I/O functions use of the streams' internals (a friend of both).
struct stream_access;

/// @cond
template <class T>
using read_hook_t = decltype(cp_read(std::declval<io::reader&>(), std::declval<T&>()));
template <class T>
using write_hook_t = decltype(cp_write(std::declval<io::writer&>(), std::declval<T const&>()));
/// @endcond

/// @brief True if `cp_read(cp::io::reader&, T&)` is found for `T`.
template <class T>
inline constexpr bool has_read_hook_v = traits::is_detected_v<read_hook_t, T>;
/// @brief True if `cp_write(cp::io::writer&, T const&)` is found for `T`.
template <class T>
inline constexpr bool has_write_hook_v = traits::is_detected_v<write_hook_t, T>;

/// @brief Type of `std::tuple_size<T>::value`; ill-formed if not specialised.
template <class T>
using tuple_size_t = decltype(std::tuple_size<T>::value);  // NOLINT(modernize-type-traits): must stay SFINAE-friendly
/// @brief True if `std::tuple_size<T>` is specialised: `std::pair`, `std::tuple`, user tuple-likes.
template <class T>
inline constexpr bool is_tuple_like_v = traits::is_detected_v<tuple_size_t, T>;

/// @brief True for what cp writes as text: `std::string`, `std::string_view`, `char const*`, `char` arrays.
template <class T>
inline constexpr bool is_string_like_v = std::is_convertible_v<T const&, std::string_view>;

/// @brief True for plain structs that get automatic I/O.
template <class T>
inline constexpr bool is_plain_aggregate_v = std::is_aggregate_v<T> && !std::is_union_v<T> && !std::is_array_v<T>;

/// @brief The buffer size of cp's reader and writer unless the constructor is given another: 64 KiB.
inline constexpr std::size_t default_buffer_size = std::size_t{1} << 16;

/// @brief How cp's reader and writer handle a type.
enum class io_kind : unsigned char {
    formatter,
    formatted,
    hook,
    boolean,
    character,
    string,
    integer,
    floating,
    range,
    tuple,
    aggregate,
    none
};

/// @brief The io_kind of `T` for the writer (`Write`) or the reader, tested in this order. The directions differ in
/// three places: each has its own hook; the writer prints anything string-like (literals, `char` arrays) as text while
/// the reader reads text into `std::string` only (a `char` array reads as single characters); the writer takes const
/// ranges.
template <class T, bool Write>
constexpr auto io_kind_of() noexcept -> io_kind {
    if constexpr (is_formatter_v<T>) {
        return io_kind::formatter;
    } else if constexpr (is_formatted_v<T>) {
        return io_kind::formatted;
    } else if constexpr (Write ? has_write_hook_v<T> : has_read_hook_v<T>) {
        return io_kind::hook;
    } else if constexpr (std::is_same_v<T, bool>) {
        return io_kind::boolean;
    } else if constexpr (std::is_same_v<T, char>) {
        return io_kind::character;
    } else if constexpr (Write ? is_string_like_v<T> : std::is_same_v<T, std::string>) {
        return io_kind::string;
    } else if constexpr (is_integer_v<T>) {
        return io_kind::integer;
    } else if constexpr (std::is_floating_point_v<T>) {
        return io_kind::floating;
    } else if constexpr (is_range_v<std::conditional_t<Write, T const, T>>) {
        return io_kind::range;
    } else if constexpr (is_tuple_like_v<T>) {
        return io_kind::tuple;
    } else if constexpr (is_plain_aggregate_v<T>) {
        return io_kind::aggregate;
    } else {
        return io_kind::none;
    }
}

/// @brief Calls `fn(get<I>(t))` for each `I`.
template <class T, class F, std::size_t... I>
constexpr void for_each_element(T& t, F& fn, std::index_sequence<I...> /*unused*/) {
    using std::get;
    (fn(get<I>(t)), ...);
}
/// @brief Calls `fn(element)` for each element of tuple-like `t` (`get` found by argument-dependent lookup).
template <class T, class F>
constexpr void for_each_element(T& t, F&& fn) {
    for_each_element(t, fn, std::make_index_sequence<std::tuple_size_v<std::remove_const_t<T>>>{});
}

template <class T, class... Seen>
constexpr auto nesting_level() noexcept -> std::size_t;

/// @brief Used for its result type: the highest nesting level among its arguments' types.
template <class... Seen>
struct max_level_fn {
    /// @brief The level, as a type (defined, so Clang accepts it for types with internal linkage).
    template <class... F>
    constexpr auto operator()(F&... /*fields*/) const noexcept
        -> std::integral_constant<std::size_t,
                                  std::max({std::size_t{0}, nesting_level<std::remove_cv_t<F>, Seen...>()...})> {
        return {};
    }
};

/// @brief The highest nesting level among the element types of tuple-like `T`.
template <class T, class... Seen, std::size_t... I>
constexpr auto tuple_level(std::index_sequence<I...> /*unused*/) noexcept -> std::size_t {
    return std::max({std::size_t{0}, nesting_level<traits::remove_cvref_t<std::tuple_element_t<I, T>>, Seen...>()...});
}

/// @brief How deeply `T` nests ranges and records: 0 for single values, one above its elements for a range, one above
/// its highest field for a tuple-like or plain struct. It picks the range separator. `Seen` are the types whose level
/// is being computed: a type met again inside itself (a tree node) counts as 0 there.
template <class T, class... Seen>
constexpr auto nesting_level() noexcept -> std::size_t {
    constexpr auto kind = io_kind_of<T, true>();
    if constexpr ((std::is_same_v<T, Seen> || ...) ||
                  (kind != io_kind::range && kind != io_kind::tuple && kind != io_kind::aggregate)) {
        return 0;  // a single value, or a type met again inside itself
    } else if constexpr (kind == io_kind::range) {
        return 1 + nesting_level<traits::remove_cvref_t<range_value_t<T const>>, T, Seen...>();
    } else if constexpr (kind == io_kind::tuple) {
        return 1 + tuple_level<T, T, Seen...>(std::make_index_sequence<std::tuple_size_v<T>>{});
    } else {  // a plain struct
        return 1 + decltype(with_fields(std::declval<T&>(), max_level_fn<T, Seen...>{}))::value;
    }
}

/// @brief nesting_level<T>().
template <class T>
inline constexpr std::size_t nesting_level_v = nesting_level<T>();

}  // namespace cp::detail
