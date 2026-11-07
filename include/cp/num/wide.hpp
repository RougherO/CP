#pragma once
/// @file
/// @brief Exact add, subtract and multiply: cp::num::add_wide, cp::num::sub_wide and cp::num::mul_wide return the full
/// result as cp::num::wide<T> (low and high halves), so the caller decides what overflow means: wrap, saturate, throw
/// or widen. cp::num::widen_t<T> is the integer type twice as wide as T, at any width (u64 → u128 → 256 bits → …).
/// @par Caveats
/// - Invariant: `op_wide(a, b).widen() == widen_t<T>(a) op widen_t<T>(b)`. For an unsigned subtraction that goes
///   below zero, `high` is all ones (the borrow), as the wrapped wide subtraction gives.
/// - Above 128 bits widen_t is a cp::detail::double_uint / double_int; see that header's caveats.
#include "cp/config.hpp"
#include "cp/detail/double_int.hpp"
#include "cp/num/int128.hpp"
#include "cp/traits/traits.hpp"
#include <cstdint>
#include <limits>
#include <type_traits>

namespace cp::detail {

/// @brief All ones if `x` is negative, else zero: the high half that sign-extends `x`.
template <class T>
constexpr auto sign_fill(T x) noexcept -> T {
    return x < 0 ? static_cast<T>(~T{}) : T{};
}

/// @brief The standard integer type of `Width` bits.
template <int Width, bool Signed>
using fixed_int_t =
    std::conditional_t<Width == 16, std::conditional_t<Signed, std::int16_t, std::uint16_t>,
                       std::conditional_t<Width == 32, std::conditional_t<Signed, std::int32_t, std::uint32_t>,
                                          std::conditional_t<Signed, std::int64_t, std::uint64_t>>>;

/// @brief Computes cp::num::widen_t; has no `type` for non-integers.
template <class T, class = void>
struct widen { };
/// @cond
template <class T>
struct widen<T, std::enable_if_t<is_integer_v<T> && (width_v<T> < 64)>> {
    using type = fixed_int_t<2 * width_v<T>, std::numeric_limits<T>::is_signed>;
};
template <class T>
struct widen<T, std::enable_if_t<is_integer_v<T> && width_v<T> == 64>> {
    using type = std::conditional_t<std::numeric_limits<T>::is_signed, num::i128, num::u128>;
};
template <class T>
struct widen<T, std::enable_if_t<is_integer_v<T> && (width_v<T> > 64)>> {
    using type = std::conditional_t<std::numeric_limits<T>::is_signed, double_int<unsigned_of_t<T>>,
                                    double_uint<unsigned_of_t<T>>>;
};
/// @endcond

}  // namespace cp::detail

namespace cp::num {

/// @brief The integer type twice as wide as `T`, with the same signedness: u32 → u64, u64 → cp::num::u128,
/// cp::num::u128 → 256 bits, and so on.
template <class T>
using widen_t = typename detail::widen<T>::type;

/// @brief An exact result split in two: `high:low` is the result as a two's-complement integer of twice T's width.
template <class T>
struct wide {
    static_assert(detail::is_integer_v<T>, "cp::num::wide requires an integer type");

    T low;   ///< The low half: what the plain, wrapping operation gives.
    T high;  ///< The high half: carry, borrow or the top of a product.

    /// @brief True if the exact result does not fit in `T`.
    constexpr auto overflow() const noexcept -> bool {
        return high != (std::numeric_limits<T>::is_signed ? detail::sign_fill(low) : T{});
    }
    /// @brief The exact result as cp::num::widen_t<T>.
    constexpr auto widen() const noexcept -> widen_t<T> {
        using wide_t    = widen_t<T>;
        using uwide_t   = detail::unsigned_of_t<wide_t>;
        using unarrow_t = detail::unsigned_of_t<T>;
        auto const hi   = static_cast<uwide_t>(static_cast<unarrow_t>(high));
        auto const lo   = static_cast<uwide_t>(static_cast<unarrow_t>(low));
        return static_cast<wide_t>(static_cast<uwide_t>(hi << detail::width_v<T>) | lo);
    }
};

}  // namespace cp::num

namespace cp::detail {

/// @brief Splits a value of widen_t<T> into cp::num::wide<T>.
template <class T, class W>
constexpr auto split_wide(W r) noexcept -> num::wide<T> {
    return {static_cast<T>(r), static_cast<T>(r >> width_v<T>)};
}

}  // namespace cp::detail

namespace cp::num {

// add_wide and sub_wide: one body for every width; GCC and Clang compile `low < a` and the sign test to the carry and
// overflow flags.

/// @brief `a + b`, exactly: `high` is the carry (signed: the sign extension).
template <class T>
constexpr auto add_wide(T a, T b) noexcept -> wide<T> {
    static_assert(detail::is_integer_v<T>, "cp::num::add_wide requires an integer type");
    using U        = detail::unsigned_of_t<T>;
    auto const low = static_cast<T>(static_cast<U>(a) + static_cast<U>(b));
    if constexpr (std::numeric_limits<T>::is_signed) {
        bool const over = ((a ^ low) & (b ^ low)) < 0;  // both operands differ in sign from the result
        return {low, detail::sign_fill(over ? a : low)};
    } else {
        return {low, low < a ? T{1} : T{}};
    }
}

/// @brief `a - b`, exactly: `high` is the borrow (all ones; signed: the sign extension).
template <class T>
constexpr auto sub_wide(T a, T b) noexcept -> wide<T> {
    static_assert(detail::is_integer_v<T>, "cp::num::sub_wide requires an integer type");
    using U        = detail::unsigned_of_t<T>;
    auto const low = static_cast<T>(static_cast<U>(a) - static_cast<U>(b));
    if constexpr (std::numeric_limits<T>::is_signed) {
        bool const over = ((a ^ b) & (a ^ low)) < 0;  // signs differ and the result took b's sign
        return {low, detail::sign_fill(over ? a : low)};
    } else {
        return {low, a < b ? static_cast<T>(~T{}) : T{}};
    }
}

/// @brief `a * b`, exactly: `high` is the top half of the product.
template <class T>
constexpr auto mul_wide(T a, T b) noexcept -> wide<T> {
    static_assert(detail::is_integer_v<T>, "cp::num::mul_wide requires an integer type");
    if constexpr (detail::width_v<T> < 64) {
        using W = widen_t<T>;
        return detail::split_wide<T>(static_cast<W>(static_cast<W>(a) * static_cast<W>(b)));
    } else {
        using U = detail::unsigned_of_t<T>;
        using detail::mul_full;
        U ua = static_cast<U>(a), ub = static_cast<U>(b), hi{};
        U lo = mul_full(ua, ub, hi);
        if constexpr (std::numeric_limits<T>::is_signed) {
            // Signed product from the unsigned one: subtract the other operand once per negative operand.
            if (a < T{}) hi -= ub;
            if (b < T{}) hi -= ua;
        }
        return {static_cast<T>(lo), static_cast<T>(hi)};
    }
}

}  // namespace cp::num
