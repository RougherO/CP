#pragma once
/// @file
/// @brief Backports from `<utility>`: C++20 safe integer comparison, C++23 `to_underlying`, `unreachable`.
#include "cp/compat/meta.hpp"
#include <limits>
#include <type_traits>
#include <utility>

namespace cp {

#if defined(__cpp_lib_integer_comparison_functions)  // cp:since c++20
using std::cmp_equal;
using std::cmp_greater;
using std::cmp_greater_equal;
using std::cmp_less;
using std::cmp_less_equal;
using std::cmp_not_equal;
using std::in_range;
#else
/// @brief `t == u` by mathematical value: `cmp_equal(-1, UINT_MAX)` is false.
template <class T, class U>
constexpr auto cmp_equal(T t, U u) noexcept {
    static_assert(detail::is_integer_v<T> && detail::is_integer_v<U>, "cp::cmp_* requires integer types");
    if constexpr (std::is_signed_v<T> == std::is_signed_v<U>) {
        return t == u;
    } else if constexpr (std::is_signed_v<T>) {
        return t >= 0 && std::make_unsigned_t<T>(t) == u;
    } else {
        return u >= 0 && std::make_unsigned_t<U>(u) == t;
    }
}
/// @brief `t != u` by mathematical value.
template <class T, class U>
constexpr auto cmp_not_equal(T t, U u) noexcept {
    return !cmp_equal(t, u);
}
/// @brief `t < u` by mathematical value: `cmp_less(-1, 0u)` is true.
template <class T, class U>
constexpr auto cmp_less(T t, U u) noexcept {
    static_assert(detail::is_integer_v<T> && detail::is_integer_v<U>, "cp::cmp_* requires integer types");
    if constexpr (std::is_signed_v<T> == std::is_signed_v<U>) {
        return t < u;
    } else if constexpr (std::is_signed_v<T>) {
        return t < 0 || std::make_unsigned_t<T>(t) < u;
    } else {
        return u >= 0 && t < std::make_unsigned_t<U>(u);
    }
}
/// @brief `t > u` by mathematical value.
template <class T, class U>
constexpr auto cmp_greater(T t, U u) noexcept {
    return cmp_less(u, t);
}
/// @brief `t <= u` by mathematical value.
template <class T, class U>
constexpr auto cmp_less_equal(T t, U u) noexcept {
    return !cmp_less(u, t);
}
/// @brief `t >= u` by mathematical value.
template <class T, class U>
constexpr auto cmp_greater_equal(T t, U u) noexcept {
    return !cmp_less(t, u);
}
/// @brief True if `t` is representable in `R`: `in_range<std::uint8_t>(256)` is false.
template <class R, class T>
constexpr auto in_range(T t) noexcept {
    return cmp_greater_equal(t, std::numeric_limits<R>::min()) && cmp_less_equal(t, std::numeric_limits<R>::max());
}
#endif

#if defined(__cpp_lib_to_underlying)  // cp:since c++23
using std::to_underlying;
#else
/// @brief Converts an enum to its underlying integer type.
template <class E>
constexpr auto to_underlying(E e) noexcept {
    return static_cast<std::underlying_type_t<E> >(e);
}
#endif

#if defined(__cpp_lib_unreachable)  // cp:since c++23
using std::unreachable;
#else
/// @brief Marks a point that is never reached; reaching it is undefined behaviour.
[[noreturn]] inline void unreachable() {
#  if CP_MSVC
    __assume(false);
#  else
    __builtin_unreachable();
#  endif
}
#endif

}  // namespace cp
