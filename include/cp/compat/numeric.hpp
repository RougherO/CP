#pragma once
/// @file
/// @brief Backports from `<numeric>`: C++20 `midpoint`, C++26 saturation arithmetic.
#include "cp/compat/utility.hpp"
#include "cp/wide.hpp"
#include "cp/detail/precondition.hpp"
#include <limits>
#include <numeric>
#include <type_traits>

namespace cp {

#if defined(__cpp_lib_interpolate)  // cp:since c++20
using std::midpoint;
#else
/// @brief Midpoint of two integers without overflow, rounded towards `a`.
template <class T>
constexpr auto midpoint(T a, T b) noexcept -> std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, T> {
    using U = std::make_unsigned_t<T>;
    if (a <= b) return static_cast<T>(a + static_cast<T>(U(U(b) - U(a)) / 2));
    return static_cast<T>(a - static_cast<T>(U(U(a) - U(b)) / 2));
}
/// @brief Midpoint of two floating-point values without overflow; exact when representable.
template <class T>
constexpr auto midpoint(T a, T b) noexcept -> std::enable_if_t<std::is_floating_point_v<T>, T> {
    constexpr T lo = std::numeric_limits<T>::min() * 2;
    constexpr T hi = std::numeric_limits<T>::max() / 2;
    T abs_a        = a < 0 ? -a : a;
    T abs_b        = b < 0 ? -b : b;
    if (abs_a <= hi && abs_b <= hi) return (a + b) / 2;  // cannot overflow
    if (abs_a < lo) return a + b / 2;                    // a/2 would underflow
    if (abs_b < lo) return a / 2 + b;
    return a / 2 + b / 2;
}
/// @brief Midpoint of two pointers into the same array, rounded towards `a`.
template <class T>
constexpr auto midpoint(T* a, T* b) noexcept -> std::enable_if_t<std::is_object_v<T>, T*> {
    static_assert(sizeof(T) != 0, "cp::midpoint requires a complete type");
    return a + (b - a) / 2;
}
#endif

// Final C++26 names (202603L); libraries with only the draft names (add_sat, 202311L) get the backport.
#if defined(__cpp_lib_saturation_arithmetic) && __cpp_lib_saturation_arithmetic >= 202603L  // cp:since c++26
using std::saturating_add;
using std::saturating_cast;
using std::saturating_div;
using std::saturating_mul;
using std::saturating_sub;
#else
namespace detail {
/// @brief The exact result clamped to `T`. Signed: the sign of `high` says which end. Unsigned: only a subtraction's
/// borrow makes `high` all ones (a carry is 1, a product's top half at most max - 1), so that means below zero.
template <class T>
constexpr auto saturate(wide<T> r) noexcept -> T {
    if (!r.overflow()) return r.low;
    bool below = std::is_signed_v<T> ? r.high < T{} : r.high == static_cast<T>(~T{});
    return below ? std::numeric_limits<T>::min() : std::numeric_limits<T>::max();
}
}  // namespace detail

/// @brief `x + y`, clamped to the range of `T`.
template <class T>
constexpr auto saturating_add(T x, T y) noexcept {
    static_assert(detail::is_integer_v<T>, "cp::saturating_add requires an integer type");
    return detail::saturate(add_wide(x, y));
}
/// @brief `x - y`, clamped to the range of `T`.
template <class T>
constexpr auto saturating_sub(T x, T y) noexcept {
    static_assert(detail::is_integer_v<T>, "cp::saturating_sub requires an integer type");
    return detail::saturate(sub_wide(x, y));
}
/// @brief `x * y`, clamped to the range of `T`.
template <class T>
constexpr auto saturating_mul(T x, T y) noexcept {
    static_assert(detail::is_integer_v<T>, "cp::saturating_mul requires an integer type");
    return detail::saturate(mul_wide(x, y));
}
/// @brief `x / y`, clamped: only `min / -1` saturates (to `max`).
/// @pre `y != 0`.
template <class T>
constexpr auto saturating_div(T x, T y) noexcept {
    static_assert(detail::is_integer_v<T>, "cp::saturating_div requires an integer type");
    detail::precondition(y != 0, "saturating_div: division by zero");
    if constexpr (std::is_signed_v<T>) {
        if (x == std::numeric_limits<T>::min() && y == -1) return std::numeric_limits<T>::max();
    }
    return static_cast<T>(x / y);
}
/// @brief `x` converted to `R`, clamped to the range of `R`.
template <class R, class T>
constexpr auto saturating_cast(T x) noexcept {
    static_assert(detail::is_integer_v<R> && detail::is_integer_v<T>, "cp::saturating_cast requires integer types");
    if (cmp_less(x, std::numeric_limits<R>::min())) return std::numeric_limits<R>::min();
    if (cmp_greater(x, std::numeric_limits<R>::max())) return std::numeric_limits<R>::max();
    return static_cast<R>(x);
}
#endif

}  // namespace cp
