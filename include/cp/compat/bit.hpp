#pragma once
/// @file
/// @brief Backport of C++20 `<bit>` and C++23 `byteswap`; unsigned types only, as in std.
/// MSVC intrinsics are not constexpr, so it uses `detail::*_portable` at compile time.
#include "cp/compat/meta.hpp"
#include <cstdint>
#include <limits>
#include <type_traits>
#if defined(__cpp_lib_bitops) || defined(__cpp_lib_bit_cast) || defined(__cpp_lib_int_pow2) || \
    defined(__cpp_lib_endian)  // cp:since c++20
#  include <bit>
#endif
#if CP_MSVC
#  include <intrin.h>
#endif

namespace cp {

namespace detail {

/// @brief Types `<bit>` accepts: unsigned integers except `bool` and character types.
template <class T>
inline constexpr bool is_bit_unsigned_v = is_integer_v<T> && std::is_unsigned_v<T>;

/// @name Portable fallbacks (MSVC compile time; tested against the intrinsics)
/// @{

/// @brief See cp::countl_zero.
template <class T>
constexpr auto countl_zero_portable(T x) noexcept {
    int n = 0;
    for (int i = digits_v<T> - 1; i >= 0 && !((x >> i) & 1u); --i) ++n;
    return n;
}
/// @brief See cp::countr_zero.
template <class T>
constexpr auto countr_zero_portable(T x) noexcept {
    int n = 0;
    for (int i = 0; i < digits_v<T> && !((x >> i) & 1u); ++i) ++n;
    return n;
}
/// @brief See cp::popcount.
template <class T>
constexpr auto popcount_portable(T x) noexcept {
    int n = 0;
    for (; x; x = static_cast<T>(x & (x - 1u))) ++n;
    return n;
}
/// @brief See cp::byteswap.
template <class T>
constexpr auto byteswap_portable(T x) noexcept {
    T r = 0;
    for (std::size_t i = 0; i < sizeof(T); ++i) {
        r = static_cast<T>((r << 8) | (x & 0xFFu));
        if constexpr (sizeof(T) > 1) x = static_cast<T>(x >> 8);
    }
    return r;
}
/// @}

}  // namespace detail

#if defined(__cpp_lib_bit_cast)  // cp:since c++20
using std::bit_cast;
#else
/// @brief Reinterprets the bits of `from` as `To` (same size, trivially copyable).
template <class To, class From>
constexpr auto bit_cast(From const& from) noexcept -> std::enable_if_t<
    sizeof(To) == sizeof(From) && std::is_trivially_copyable_v<To> && std::is_trivially_copyable_v<From>, To> {
    return __builtin_bit_cast(To, from);
}
#endif

#if defined(__cpp_lib_bitops)  // cp:since c++20
using std::countl_one;
using std::countl_zero;
using std::countr_one;
using std::countr_zero;
using std::popcount;
using std::rotl;
using std::rotr;
#else
/// @brief Leading 0 bits; bit width of `T` when `x == 0`.
template <class T>
constexpr auto countl_zero(T x) noexcept -> int {
    static_assert(detail::is_bit_unsigned_v<T>, "cp::countl_zero requires an unsigned integer type");
    if (x == 0) return detail::digits_v<T>;
    if constexpr (detail::digits_v<T> > 64) {  // unsigned __int128: one 64-bit count per half
        auto hi = static_cast<std::uint64_t>(x >> 64);
        return hi != 0 ? countl_zero(hi) : 64 + countl_zero(static_cast<std::uint64_t>(x));
    }
#  if CP_MSVC
    if (is_constant_evaluated()) return detail::countl_zero_portable(x);
    unsigned long idx;
    _BitScanReverse64(&idx, static_cast<std::uint64_t>(x));
    return detail::digits_v<T> - 1 - static_cast<int>(idx);
#  else
    // subtract the zero bits above a narrow T
    return __builtin_clzll(static_cast<unsigned long long>(x)) - (64 - detail::digits_v<T>);
#  endif
}
/// @brief Trailing 0 bits; bit width of `T` when `x == 0`.
template <class T>
constexpr auto countr_zero(T x) noexcept -> int {
    static_assert(detail::is_bit_unsigned_v<T>, "cp::countr_zero requires an unsigned integer type");
    if (x == 0) return detail::digits_v<T>;
    if constexpr (detail::digits_v<T> > 64) {  // unsigned __int128: one 64-bit count per half
        auto lo = static_cast<std::uint64_t>(x);
        return lo != 0 ? countr_zero(lo) : 64 + countr_zero(static_cast<std::uint64_t>(x >> 64));
    }
#  if CP_MSVC
    if (is_constant_evaluated()) return detail::countr_zero_portable(x);
    unsigned long idx;
    _BitScanForward64(&idx, static_cast<std::uint64_t>(x));
    return static_cast<int>(idx);
#  else
    return __builtin_ctzll(static_cast<unsigned long long>(x));
#  endif
}
/// @brief Leading 1 bits.
template <class T>
constexpr auto countl_one(T x) noexcept {
    return countl_zero(static_cast<T>(~x));
}
/// @brief Trailing 1 bits.
template <class T>
constexpr auto countr_one(T x) noexcept {
    return countr_zero(static_cast<T>(~x));
}
/// @brief Number of 1 bits.
template <class T>
constexpr auto popcount(T x) noexcept -> int {
    static_assert(detail::is_bit_unsigned_v<T>, "cp::popcount requires an unsigned integer type");
    if constexpr (detail::digits_v<T> > 64) {  // unsigned __int128: one 64-bit count per half
        return popcount(static_cast<std::uint64_t>(x)) + popcount(static_cast<std::uint64_t>(x >> 64));
    }
#  if CP_MSVC
    if (is_constant_evaluated()) return detail::popcount_portable(x);
    return static_cast<int>(__popcnt64(static_cast<std::uint64_t>(x)));
#  else
    return __builtin_popcountll(static_cast<unsigned long long>(x));
#  endif
}
template <class T>
[[nodiscard]] constexpr auto rotr(T x, int s) noexcept -> T;
/// @brief Rotate left by `s` (any `int`; negative rotates right).
template <class T>
[[nodiscard]] constexpr auto rotl(T x, int s) noexcept -> T {
    static_assert(detail::is_bit_unsigned_v<T>, "cp::rotl requires an unsigned integer type");
    constexpr int n = detail::digits_v<T>;
    int r           = s % n;
    if (r == 0) return x;
    if (r < 0) return rotr(x, -r);
    return static_cast<T>((x << r) | (x >> (n - r)));
}
/// @brief Rotate right by `s` (any `int`; negative rotates left).
template <class T>
[[nodiscard]] constexpr auto rotr(T x, int s) noexcept -> T {
    static_assert(detail::is_bit_unsigned_v<T>, "cp::rotr requires an unsigned integer type");
    constexpr int n = detail::digits_v<T>;
    int r           = s % n;
    if (r == 0) return x;
    if (r < 0) return rotl(x, -r);
    return static_cast<T>((x >> r) | (x << (n - r)));
}
#endif

#if defined(__cpp_lib_int_pow2)  // cp:since c++20
using std::bit_ceil;
using std::bit_floor;
using std::bit_width;
using std::has_single_bit;
#else
/// @brief True if `x` is a power of two.
template <class T>
constexpr auto has_single_bit(T x) noexcept {
    static_assert(detail::is_bit_unsigned_v<T>, "cp::has_single_bit requires an unsigned integer type");
    return x != 0 && (x & (x - 1u)) == 0;
}
/// @brief `1 + floor(log2(x))`; 0 for 0.
template <class T>
constexpr auto bit_width(T x) noexcept {
    return detail::digits_v<T> - countl_zero(x);
}
/// @brief Largest power of two `<= x`; 0 for 0.
template <class T>
constexpr auto bit_floor(T x) noexcept {
    return x == 0 ? T{0} : static_cast<T>(T{1} << (bit_width(x) - 1));
}
/// @brief Smallest power of two `>= x`; 1 for 0.
/// @pre The result fits in `T`.
template <class T>
constexpr auto bit_ceil(T x) noexcept {
    return x <= 1u ? T{1} : static_cast<T>(T{1} << bit_width(static_cast<T>(x - 1u)));
}
#endif

#if defined(__cpp_lib_endian)  // cp:since c++20
using std::endian;
#else
/// @brief Byte order; `native` is `little` or `big`.
enum class endian {
#  if CP_MSVC
    little = 0,
    big    = 1,
    native = little
#  else
    little = __ORDER_LITTLE_ENDIAN__,
    big    = __ORDER_BIG_ENDIAN__,
    native = __BYTE_ORDER__
#  endif
};
#endif

#if defined(__cpp_lib_byteswap)  // cp:since c++23
using std::byteswap;
#else
/// @brief Reverses the bytes of an integer.
template <class T>
constexpr auto byteswap(T x) noexcept {
    static_assert(std::is_integral_v<T>, "cp::byteswap requires an integer type");
    using U = std::make_unsigned_t<T>;
    auto u  = static_cast<U>(x);
    if constexpr (sizeof(T) == 1) {
        return x;
#  if !CP_MSVC
    } else if constexpr (sizeof(T) == 2) {
        return static_cast<T>(__builtin_bswap16(u));
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(__builtin_bswap32(u));
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>(__builtin_bswap64(u));
#  endif
    } else {
        return static_cast<T>(detail::byteswap_portable(u));
    }
}
#endif

}  // namespace cp
