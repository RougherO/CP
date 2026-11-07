#pragma once
/// @file
/// @brief cp::num::cmp: three-way comparison of integers by mathematical value, whatever their signedness, and of
/// floating-point values (NaN unordered); the ordering types; cp::num::fits: whether a value fits a type.
/// @code
/// num::cmp(-1, 0u) < 0;              // true: -1 is less than 0 (the plain -1 < 0u is false)
/// num::cmp(a, b) == num::strong_ordering::equal;
/// num::cmp(x, NAN) == num::partial_ordering::unordered;
/// num::fits<std::uint8_t>(300);      // false
/// @endcode
/// @par Caveats
/// - Integers of any width (not `bool` or character types), `__int128` and cp's wide integers included.
/// - num::strong_ordering, weak_ordering and partial_ordering are the std types from C++20; in C++17 cp types with
///   the same use: compare with literal `0` (`< 0`, `== 0`, …) or with the constants (`strong_ordering::less`, …),
///   and strong converts to weak converts to partial.
#include "cp/config.hpp"
#include "cp/traits/traits.hpp"
#include <limits>
#include <type_traits>
#if defined(__cpp_lib_three_way_comparison)  // cp:since c++20
#  include <compare>
#endif

#if !defined(__cpp_lib_three_way_comparison)  // C++17: cp's ordering types; C++20 uses std's below
namespace cp::detail {

/// @brief Only a literal 0 converts to this, so `ordering < 0` compiles and `ordering < 1` does not.
struct ordering_zero {
    /// @brief From a null pointer constant.
    constexpr ordering_zero(ordering_zero*) noexcept { }  // NOLINT(google-explicit-constructor): accepts literal 0
};

/// @brief The comparisons every ordering type shares, against literal 0 and against itself. `m_value` is -1, 0 or 1,
/// or 2 for unordered, which compares false with 0 except under `!=`.
template <class D>
class ordering_base {
public:
    ///@{
    /// @brief Comparisons against the same type and against literal 0.
    friend constexpr auto operator==(D a, D b) noexcept -> bool { return a.m_value == b.m_value; }
    friend constexpr auto operator!=(D a, D b) noexcept -> bool { return a.m_value != b.m_value; }
    friend constexpr auto operator==(D a, ordering_zero) noexcept -> bool { return a.m_value == 0; }
    friend constexpr auto operator!=(D a, ordering_zero) noexcept -> bool { return a.m_value != 0; }
    friend constexpr auto operator<(D a, ordering_zero) noexcept -> bool { return a.m_value == -1; }
    friend constexpr auto operator<=(D a, ordering_zero) noexcept -> bool { return a.m_value == -1 || a.m_value == 0; }
    friend constexpr auto operator>(D a, ordering_zero) noexcept -> bool { return a.m_value == 1; }
    friend constexpr auto operator>=(D a, ordering_zero) noexcept -> bool { return a.m_value == 1 || a.m_value == 0; }
    friend constexpr auto operator==(ordering_zero z, D a) noexcept -> bool { return a == z; }
    friend constexpr auto operator!=(ordering_zero z, D a) noexcept -> bool { return a != z; }
    friend constexpr auto operator<(ordering_zero z, D a) noexcept -> bool { return a > z; }
    friend constexpr auto operator<=(ordering_zero z, D a) noexcept -> bool { return a >= z; }
    friend constexpr auto operator>(ordering_zero z, D a) noexcept -> bool { return a < z; }
    friend constexpr auto operator>=(ordering_zero z, D a) noexcept -> bool { return a <= z; }
    ///@}

protected:
    /// @brief Holds `v`.
    constexpr explicit ordering_base(signed char v) noexcept : m_value(v) { }
    /// @brief -1, 0, 1, or 2 for unordered.
    signed char m_value;
};

}  // namespace cp::detail

namespace cp::num {

/// @brief Result of a comparison where some values are unordered (NaN): less, equivalent, greater or unordered.
class partial_ordering : public detail::ordering_base<partial_ordering> {
public:
    ///@{
    /// @brief The values, as for `std::partial_ordering`.
    static partial_ordering const less;
    static partial_ordering const equivalent;
    static partial_ordering const greater;
    static partial_ordering const unordered;
    ///@}

private:
    friend class weak_ordering;
    friend class strong_ordering;
    using ordering_base::ordering_base;
};
inline constexpr partial_ordering partial_ordering::less{-1};
inline constexpr partial_ordering partial_ordering::equivalent{0};
inline constexpr partial_ordering partial_ordering::greater{1};
inline constexpr partial_ordering partial_ordering::unordered{2};

/// @brief Result of a comparison where equivalent values may differ: less, equivalent or greater.
class weak_ordering : public detail::ordering_base<weak_ordering> {
public:
    ///@{
    /// @brief The values, as for `std::weak_ordering`.
    static weak_ordering const less;
    static weak_ordering const equivalent;
    static weak_ordering const greater;
    ///@}

    /// @brief Same value as a partial_ordering.
    // NOLINTNEXTLINE(google-explicit-constructor): converts implicitly, as std::weak_ordering does
    constexpr operator partial_ordering() const noexcept { return partial_ordering(m_value); }

private:
    friend class strong_ordering;
    using ordering_base::ordering_base;
};
inline constexpr weak_ordering weak_ordering::less{-1};
inline constexpr weak_ordering weak_ordering::equivalent{0};
inline constexpr weak_ordering weak_ordering::greater{1};

/// @brief Result of a comparison where equal values are interchangeable: less, equal or greater.
class strong_ordering : public detail::ordering_base<strong_ordering> {
public:
    ///@{
    /// @brief The values, as for `std::strong_ordering`.
    static strong_ordering const less;
    static strong_ordering const equal;
    static strong_ordering const equivalent;
    static strong_ordering const greater;
    ///@}

    ///@{
    /// @brief Same value as a weaker ordering.
    // NOLINTNEXTLINE(google-explicit-constructor): converts implicitly, as std::strong_ordering does
    constexpr operator partial_ordering() const noexcept { return partial_ordering(m_value); }
    // NOLINTNEXTLINE(google-explicit-constructor)
    constexpr operator weak_ordering() const noexcept { return weak_ordering(m_value); }
    ///@}

private:
    using ordering_base::ordering_base;
};
inline constexpr strong_ordering strong_ordering::less{-1};
inline constexpr strong_ordering strong_ordering::equal{0};
inline constexpr strong_ordering strong_ordering::equivalent{0};
inline constexpr strong_ordering strong_ordering::greater{1};

}  // namespace cp::num
#endif

namespace cp::detail {

/// @brief `t < u` by mathematical value for any two integers. `std::cmp_less` does the same for standard types only
/// (not `__int128` or cp's wide integers), so cp uses this one rule for all.
template <class T, class U>
constexpr auto cmp_less(T t, U u) noexcept -> bool {
    if constexpr (std::numeric_limits<T>::is_signed == std::numeric_limits<U>::is_signed) {
        return t < u;
    } else if constexpr (std::numeric_limits<T>::is_signed) {
        return t < 0 || static_cast<unsigned_of_t<T>>(t) < u;
    } else {
        return u >= 0 && t < static_cast<unsigned_of_t<U>>(u);
    }
}

}  // namespace cp::detail

/// @namespace cp::num
/// @brief Integers: 128-bit and wider types, exact and saturating arithmetic, comparison, modular integers.
namespace cp::num {

#if defined(__cpp_lib_three_way_comparison)  // cp:since c++20
using std::partial_ordering;
using std::strong_ordering;
using std::weak_ordering;
#endif

/// @brief Integers `t` against `u` by mathematical value: `cmp(-1, 0u) < 0`, `cmp(-1, UINT_MAX) != 0`.
/// Floating-point values give a partial_ordering (NaN is unordered with everything).
template <class T, class U>
constexpr auto cmp(T t, U u) noexcept {
    static_assert((detail::is_integer_v<T> && detail::is_integer_v<U>) ||
                      (std::is_floating_point_v<T> && std::is_floating_point_v<U>),
                  "cp::num::cmp compares two integers or two floating-point values");
    if constexpr (std::is_floating_point_v<T>) {
#if defined(__cpp_lib_three_way_comparison)  // cp:since c++20
        return t <=> u;
#else
        return t < u  ? partial_ordering::less
             : t > u  ? partial_ordering::greater
             : t == u ? partial_ordering::equivalent
                      : partial_ordering::unordered;
#endif
    } else {
        bool const less = detail::cmp_less(t, u), greater = detail::cmp_less(u, t);
        return less ? strong_ordering::less : greater ? strong_ordering::greater : strong_ordering::equal;
    }
}

/// @brief True if `t` is representable in `R`: `fits<std::uint8_t>(256)` is false.
template <class R, class T>
constexpr auto fits(T t) noexcept -> bool {
    static_assert(detail::is_integer_v<R> && detail::is_integer_v<T>, "cp::num::fits requires integer types");
    return !detail::cmp_less(t, std::numeric_limits<R>::min()) && !detail::cmp_less(std::numeric_limits<R>::max(), t);
}

}  // namespace cp::num
