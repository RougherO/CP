#pragma once
/// @file
/// @brief Field names of plain structs, for the `field_names` formatter: cp::detail::field_names_v and
/// cp::detail::has_field_names_v.
/// @par Caveats
/// - C++26 with reflection (`__cpp_impl_reflection`): `std::meta::identifier_of`.
/// - C++20 (`__cpp_nontype_template_args >= 201911L`; Clang, which still reports 201411, from version 23, the oldest
///   verified): a pointer to each field of a declared-only object is a template argument, and the name is cut from
///   `__PRETTY_FUNCTION__` at compile time (the Boost.PFR technique). Only the names reach the binary.
/// - C++17: no names (pointers to subobjects cannot be template arguments); has_field_names_v is false. Before
///   C++26 reflection, a struct with a bit-field has no names either (a bit-field has no address).
#include "cp/config.hpp"
#include "cp/io/aggregate.hpp"
#include <array>
#include <string_view>

#if defined(__cpp_impl_reflection)  // cp:since c++26
#  include <cstddef>
#  include <meta>
#  include <type_traits>

namespace cp::detail {

/// @brief True if field_names_v<T> holds the names.
template <class T>
inline constexpr bool has_field_names_v = true;

/// @brief The names of `T`'s fields, in declaration order.
template <class T>
inline constexpr auto field_names_v = [] {
    std::array<std::string_view, field_count_v<T>> names{};
    std::size_t i = 0;
    template for (constexpr auto m : std::define_static_array(std::meta::nonstatic_data_members_of(
                      ^^std::remove_cv_t<T>, std::meta::access_context::unchecked()))) names[i++] =
        std::meta::identifier_of(m);
    return names;
}();

}  // namespace cp::detail

#elif (defined(__cpp_nontype_template_args) && __cpp_nontype_template_args >= 201911L) || \
    (defined(__clang__) && __cplusplus >= 202002L && __clang_major__ >= 23)  // cp:since c++20
#  include <algorithm>
#  include <cstddef>
#  include <tuple>
#  include <type_traits>
#  include <utility>

namespace cp::detail {

/// @brief Holds a `T` without constructing one.
template <class T>
struct field_name_holder {
    T const value;  ///< Never read.
};

#  if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wundefined-var-template"
#    pragma clang diagnostic ignored "-Wundefined-internal"
#  endif

/// @brief Declared, never defined: only the addresses of its fields are used, inside constant expressions.
template <class T>
extern field_name_holder<T> const field_name_object;

/// @brief Pointers to the fields of field_name_object<T>.
template <class T>
constexpr auto field_pointers() noexcept {
    return with_fields(field_name_object<T>.value, [](auto const&... f) { return std::tuple{&f...}; });
}

#  if defined(__clang__)
#    pragma clang diagnostic pop
#  endif

/// @brief The signature of this function, which spells out `P`: `… P = (& obj.….T::name); …` (GCC) or
/// `… [P = &obj.….name]` (Clang).
template <auto P>
constexpr auto pointer_signature() noexcept -> std::string_view {
    return __PRETTY_FUNCTION__;
}

/// @brief The field name at the end of a pointer_signature.
constexpr auto field_name_in(std::string_view sig) noexcept -> std::string_view {
    std::size_t const start = sig.find("P = ");
    std::size_t end         = std::min(sig.find(';', start), sig.rfind(']'));
    auto const is_ident     = [](char c) {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
    };
    while (end > start && !is_ident(sig[end - 1])) --end;  // a closing ')' (GCC)
    std::size_t begin = end;
    while (begin > start && is_ident(sig[begin - 1])) --begin;
    return sig.substr(begin, end - begin);
}

/// @brief The names of `T`'s fields, `I...` being their indices.
template <class T, std::size_t... I>
constexpr auto field_names_of(std::index_sequence<I...> /*unused*/) noexcept
    -> std::array<std::string_view, sizeof...(I)> {
    return {field_name_in(pointer_signature<std::get<I>(field_pointers<T>())>())...};
}

/// @brief True if field_names_v<T> holds the names: every field has an address (no bit-fields).
template <class T>
inline constexpr bool has_field_names_v =
    requires { typename std::integral_constant<bool, (static_cast<void>(field_pointers<T>()), true)>; };

/// @brief The names of `T`'s fields, in declaration order.
template <class T>
inline constexpr auto field_names_v = field_names_of<T>(std::make_index_sequence<field_count_v<T>>{});

}  // namespace cp::detail

#else

namespace cp::detail {

/// @brief True if field_names_v<T> holds the names: never before C++20.
template <class T>
inline constexpr bool has_field_names_v = false;

/// @brief Empty names, before C++20.
template <class T>
inline constexpr std::array<std::string_view, field_count_v<T>> field_names_v{};

}  // namespace cp::detail

#endif
