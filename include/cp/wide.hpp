#pragma once
/// @file
/// @brief Exact add, subtract and multiply: cp::add_wide, cp::sub_wide and cp::mul_wide return the full result as
/// cp::wide<T> (low and high halves), so the caller decides what overflow means: wrap, saturate, throw or widen.
/// cp::widen_t<T> is the integer type twice as wide as T, at any width (u64 → u128 → 256 bits → …).
/// @par Caveats
/// - Invariant: `op_wide(a, b).widen() == widen_t<T>(a) op widen_t<T>(b)`. For an unsigned subtraction that goes
///   below zero, `high` is all ones (the borrow), as the wrapped wide subtraction gives.
/// - Above 128 bits widen_t is a cp::detail::double_uint / double_int; see that header's caveats.
#include "cp/config.hpp"
#include "cp/detail/double_int.hpp"
#include "cp/int128.hpp"
#include "cp/compat/meta.hpp"
#include <cstdint>
#include <limits>
#include <type_traits>

namespace cp {

namespace detail {

/// @brief The standard integer type of `Width` bits.
template <int Width, bool Signed>
using fixed_int_t =
    std::conditional_t<Width == 16, std::conditional_t<Signed, std::int16_t, std::uint16_t>,
                       std::conditional_t<Width == 32, std::conditional_t<Signed, std::int32_t, std::uint32_t>,
                                          std::conditional_t<Signed, std::int64_t, std::uint64_t>>>;

/// @brief Computes cp::widen_t; has no `type` for non-integers.
template <class T, class = void>
struct widen { };
/// @cond
template <class T>
struct widen<T, std::enable_if_t<is_int_like_v<T> && (width_v<T> < 64)>> {
    using type = fixed_int_t<2 * width_v<T>, std::numeric_limits<T>::is_signed>;
};
template <class T>
struct widen<T, std::enable_if_t<is_int_like_v<T> && width_v<T> == 64>> {
    using type = std::conditional_t<std::numeric_limits<T>::is_signed, i128, u128>;
};
template <class T>
struct widen<T, std::enable_if_t<is_int_like_v<T> && (width_v<T> > 64)>> {
    using type = std::conditional_t<std::numeric_limits<T>::is_signed, double_int<unsigned_of_t<T>>,
                                    double_uint<unsigned_of_t<T>>>;
};
/// @endcond

}  // namespace detail

/// @brief The integer type twice as wide as `T`, with the same signedness: u32 → u64, u64 → cp::u128, cp::u128 →
/// 256 bits, and so on.
template <class T>
using widen_t = typename detail::widen<T>::type;

/// @brief An exact result split in two: `high:low` is the result as a two's-complement integer of twice T's width.
template <class T>
struct wide {
    static_assert(detail::is_int_like_v<T>, "cp::wide requires an integer type");

    T low;   ///< The low half: what the plain, wrapping operation gives.
    T high;  ///< The high half: carry, borrow or the top of a product.

    /// @brief True if the exact result does not fit in `T`.
    constexpr auto overflow() const noexcept -> bool {
        if constexpr (std::numeric_limits<T>::is_signed) {
            return high != (low < T{} ? ~T{} : T{});
        } else {
            return high != T{};
        }
    }
    /// @brief The exact result as cp::widen_t<T>.
    constexpr auto widen() const noexcept -> widen_t<T> {
        using wide_t    = widen_t<T>;
        using uwide_t   = detail::unsigned_of_t<wide_t>;
        using unarrow_t = detail::unsigned_of_t<T>;
        auto const hi   = static_cast<uwide_t>(static_cast<unarrow_t>(high));
        auto const lo   = static_cast<uwide_t>(static_cast<unarrow_t>(low));
        return static_cast<wide_t>(static_cast<uwide_t>(hi << detail::width_v<T>) | lo);
    }
};

namespace detail {

/// @brief Splits a value of widen_t<T> into cp::wide<T>.
template <class T, class W>
constexpr auto split_wide(W r) noexcept -> wide<T> {
    return {static_cast<T>(r), static_cast<T>(r >> width_v<T>)};
}
/// @brief All ones if `x` is negative, else zero.
template <class T>
constexpr auto sign_fill(T x) noexcept -> T {
    return x < T{} ? ~T{} : T{};
}

}  // namespace detail

namespace detail {

/// @brief cp::add_wide without compiler builtins (MSVC, cp's double-width types); tested against the builtins.
template <class T>
constexpr auto add_wide_portable(T a, T b) noexcept -> wide<T> {
    using U  = unsigned_of_t<T>;
    auto low = static_cast<T>(static_cast<U>(a) + static_cast<U>(b));
    if constexpr (std::numeric_limits<T>::is_signed) {
        bool over = ((a ^ low) & (b ^ low)) < T{};  // both operands differ in sign from the result
        return {low, sign_fill(over ? a : low)};
    } else {
        return {low, low < a ? T{1} : T{}};
    }
}
/// @brief cp::sub_wide without compiler builtins (MSVC, cp's double-width types); tested against the builtins.
template <class T>
constexpr auto sub_wide_portable(T a, T b) noexcept -> wide<T> {
    using U  = unsigned_of_t<T>;
    auto low = static_cast<T>(static_cast<U>(a) - static_cast<U>(b));
    if constexpr (std::numeric_limits<T>::is_signed) {
        bool over = ((a ^ b) & (a ^ low)) < T{};  // operands differ in sign and the result took b's sign
        return {low, sign_fill(over ? a : low)};
    } else {
        return {low, a < b ? ~T{} : T{}};
    }
}

}  // namespace detail

/// @brief `a + b`, exactly: `high` is the carry (signed: the sign extension).
template <class T>
constexpr auto add_wide(T a, T b) noexcept -> wide<T> {
    static_assert(detail::is_int_like_v<T>, "cp::add_wide requires an integer type");
    if constexpr (detail::width_v<T> < 64) {
        using W = widen_t<T>;
        return detail::split_wide<T>(static_cast<W>(static_cast<W>(a) + static_cast<W>(b)));
#if !CP_MSVC
    } else if constexpr (!std::is_class_v<T>) {
        T low{};
        bool over = __builtin_add_overflow(a, b, &low);
        if constexpr (std::numeric_limits<T>::is_signed)
            return {low, detail::sign_fill(over ? a : low)};
        else
            return {low, over ? T{1} : T{}};
#endif
    } else {
        return detail::add_wide_portable(a, b);
    }
}

/// @brief `a - b`, exactly: `high` is the borrow (all ones; signed: the sign extension).
template <class T>
constexpr auto sub_wide(T a, T b) noexcept -> wide<T> {
    static_assert(detail::is_int_like_v<T>, "cp::sub_wide requires an integer type");
    if constexpr (detail::width_v<T> < 64) {
        using W = widen_t<T>;
        return detail::split_wide<T>(static_cast<W>(static_cast<W>(a) - static_cast<W>(b)));
#if !CP_MSVC
    } else if constexpr (!std::is_class_v<T>) {
        T low{};
        bool over = __builtin_sub_overflow(a, b, &low);
        if constexpr (std::numeric_limits<T>::is_signed)
            return {low, detail::sign_fill(over ? a : low)};
        else
            return {low, over ? ~T{} : T{}};
#endif
    } else {
        return detail::sub_wide_portable(a, b);
    }
}

/// @brief `a * b`, exactly: `high` is the top half of the product.
template <class T>
constexpr auto mul_wide(T a, T b) noexcept -> wide<T> {
    static_assert(detail::is_int_like_v<T>, "cp::mul_wide requires an integer type");
    if constexpr (detail::width_v<T> < 64) {
        using W = widen_t<T>;
        return detail::split_wide<T>(static_cast<W>(static_cast<W>(a) * static_cast<W>(b)));
    } else {
        // 64-bit types are normalised to std::uint64_t, the type the u64 primitive takes.
        using U = std::conditional_t<detail::width_v<T> == 64, std::uint64_t, detail::unsigned_of_t<T>>;
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

}  // namespace cp
