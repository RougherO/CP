#pragma once
/// @file
/// @brief cp::num arithmetic: `midpoint`, and saturating `add_sat`, `sub_sat`, `mul_sat`, `div_sat`, `cast_sat`
/// (results clamped to the type's range instead of wrapping).
/// @par Caveats
/// - Saturating functions take integers of any width (not `bool` or character types), both operands of one type.
/// - The std functions do the work when the library has them (`std::saturating_add`, C++26); with GCC 14–15's draft
///   names (`std::add_sat`) the cp names are those functions themselves.
#include "cp/config.hpp"
#include "cp/detail/precondition.hpp"
#include "cp/num/cmp.hpp"
#include "cp/num/wide.hpp"
#include "cp/traits/traits.hpp"
#include <limits>
#if defined(__cpp_lib_interpolate)  // cp:since c++20
#  include <numeric>
#endif
#if !defined(__cpp_lib_interpolate) || defined(__cpp_lib_saturation_arithmetic)  // the fallback midpoint; C++26
#  include <type_traits>
#endif

namespace cp::detail {

/// @brief The exact result clamped to `T`. Signed: the sign of `high` says which end. Unsigned: only a subtraction's
/// borrow makes `high` all ones (a carry is 1, a product's top half at most max - 1), so that means below zero.
template <class T>
constexpr auto saturate(num::wide<T> r) noexcept -> T {
    if (!r.overflow()) return r.low;
    bool const below = std::numeric_limits<T>::is_signed ? r.high < 0 : r.high == static_cast<T>(~T{});
    return below ? std::numeric_limits<T>::min() : std::numeric_limits<T>::max();
}
}  // namespace cp::detail

namespace cp::num {

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
    static_assert(sizeof(T) != 0, "cp::num::midpoint requires a complete type");
    return a + (b - a) / 2;
}
#endif

// C++26 has the final names (202603L); GCC 14–15 shipped the draft names, which are these functions exactly.
#if defined(__cpp_lib_saturation_arithmetic) && __cpp_lib_saturation_arithmetic < 202603L  // cp:since c++26
using std::add_sat;
using std::div_sat;
using std::mul_sat;
using std::sub_sat;
#else
// The std functions take standard integer types only; `__int128` and cp's wide integers use the exact result.
/// @brief `x + y`, clamped to the range of `T`.
template <class T>
constexpr auto add_sat(T x, T y) noexcept -> T {
    static_assert(detail::is_integer_v<T>, "cp::num::add_sat requires an integer type");
#  if defined(__cpp_lib_saturation_arithmetic)  // cp:since c++26
    if constexpr (std::is_integral_v<T>) return std::saturating_add(x, y);
#  endif
    return detail::saturate(add_wide(x, y));
}
/// @brief `x - y`, clamped to the range of `T`.
template <class T>
constexpr auto sub_sat(T x, T y) noexcept -> T {
    static_assert(detail::is_integer_v<T>, "cp::num::sub_sat requires an integer type");
#  if defined(__cpp_lib_saturation_arithmetic)  // cp:since c++26
    if constexpr (std::is_integral_v<T>) return std::saturating_sub(x, y);
#  endif
    return detail::saturate(sub_wide(x, y));
}
/// @brief `x * y`, clamped to the range of `T`.
template <class T>
constexpr auto mul_sat(T x, T y) noexcept -> T {
    static_assert(detail::is_integer_v<T>, "cp::num::mul_sat requires an integer type");
#  if defined(__cpp_lib_saturation_arithmetic)  // cp:since c++26
    if constexpr (std::is_integral_v<T>) return std::saturating_mul(x, y);
#  endif
    return detail::saturate(mul_wide(x, y));
}
/// @brief `x / y`, clamped: only `min / -1` saturates (to `max`).
/// @pre `y != 0`.
template <class T>
constexpr auto div_sat(T x, T y) noexcept -> T {
    static_assert(detail::is_integer_v<T>, "cp::num::div_sat requires an integer type");
    detail::precondition(y != 0, "div_sat: division by zero");
#  if defined(__cpp_lib_saturation_arithmetic)  // cp:since c++26
    if constexpr (std::is_integral_v<T>) return std::saturating_div(x, y);
#  endif
    if constexpr (std::numeric_limits<T>::is_signed) {
        if (x == std::numeric_limits<T>::min() && y == T(-1)) return std::numeric_limits<T>::max();
    }
    return static_cast<T>(x / y);
}
#endif

/// @brief `x` converted to `R`, clamped to the range of `R`.
template <class R, class T>
constexpr auto cast_sat(T x) noexcept -> R {
    static_assert(detail::is_integer_v<R> && detail::is_integer_v<T>, "cp::num::cast_sat requires integer types");
#if defined(__cpp_lib_saturation_arithmetic) && __cpp_lib_saturation_arithmetic >= 202603L  // cp:since c++26
    if constexpr (std::is_integral_v<R> && std::is_integral_v<T>) return std::saturating_cast<R>(x);
#endif
    if (fits<R>(x)) return static_cast<R>(x);
    return x < 0 ? std::numeric_limits<R>::min() : std::numeric_limits<R>::max();
}

}  // namespace cp::num
