#pragma once
/// @file
/// @brief Optional short integer names (`cp::aliases::i64`, `cp::aliases::u128`, `cp::aliases::usize`, …) and a literal
/// from 32 bits up (`5_i64`, `3_uz`, `1_u128`).
/// Not included by cp/all.hpp and not used inside the library: include it in solutions that want the names.
/// @par Caveats
/// - Literals that do not fit their type do not compile (`9223372036854775808_i64`). They take decimal, `0x`, `0b`
///   and octal digits, with `'` separators.
/// - A literal is never negative: `-5_i64` is `-(5_i64)`, as with every C++ literal.
/// - `-170141183460469231731687303715884105728_i128` does not fit; use `std::numeric_limits<i128>::min()`.
/// - No `_i8`/`_i16`/`_u8`/`_u16`: any arithmetic on them gives an `int` anyway.
/// - `_uz` duplicates C++23's built-in `uz` suffix and goes away when C++23 is the floor.
#include "cp/detail/integer.hpp"
#include "cp/num/int128.hpp"
#include <cstddef>
#include <cstdint>

namespace cp::aliases {

using i8    = std::int8_t;     ///< 8-bit signed integer.
using i16   = std::int16_t;    ///< 16-bit signed integer.
using i32   = std::int32_t;    ///< 32-bit signed integer.
using i64   = std::int64_t;    ///< 64-bit signed integer.
using u8    = std::uint8_t;    ///< 8-bit unsigned integer.
using u16   = std::uint16_t;   ///< 16-bit unsigned integer.
using u32   = std::uint32_t;   ///< 32-bit unsigned integer.
using u64   = std::uint64_t;   ///< 64-bit unsigned integer.
using num::i128;              ///< 128-bit signed integer (cp::num::i128).
using num::u128;              ///< 128-bit unsigned integer (cp::num::u128).
using isize = std::ptrdiff_t;  ///< Signed size: differences of indices and pointers.
using usize = std::size_t;     ///< Unsigned size.

inline namespace literals {

/// @brief A cp::aliases::i32 constant.
template <char... Cs>
constexpr auto operator""_i32() noexcept { return detail::int_literal<i32, Cs...>(); }
/// @brief A cp::aliases::i64 constant.
template <char... Cs>
constexpr auto operator""_i64() noexcept { return detail::int_literal<i64, Cs...>(); }
/// @brief A cp::aliases::i128 constant.
template <char... Cs>
constexpr auto operator""_i128() noexcept { return detail::int_literal<i128, Cs...>(); }
/// @brief A cp::aliases::u32 constant.
template <char... Cs>
constexpr auto operator""_u32() noexcept { return detail::int_literal<u32, Cs...>(); }
/// @brief A cp::aliases::u64 constant.
template <char... Cs>
constexpr auto operator""_u64() noexcept { return detail::int_literal<u64, Cs...>(); }
/// @brief A cp::aliases::u128 constant.
template <char... Cs>
constexpr auto operator""_u128() noexcept { return detail::int_literal<u128, Cs...>(); }
/// @brief A cp::aliases::isize constant.
template <char... Cs>
constexpr auto operator""_iz() noexcept { return detail::int_literal<isize, Cs...>(); }
/// @brief A cp::aliases::usize constant (the built-in `uz` suffix from C++23).
template <char... Cs>
constexpr auto operator""_uz() noexcept { return detail::int_literal<usize, Cs...>(); }

}  // namespace literals

}  // namespace cp::aliases
