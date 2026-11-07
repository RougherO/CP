#pragma once
/// @file
/// @brief cp::bits: bit operations on unsigned integers of any width (`unsigned __int128` and cp's wide integers
/// included).
/// @code
/// using namespace cp;
/// bits::popcnt(x);  bits::clz(x);  bits::ctz(x);  bits::width(x);  bits::ceil2(x);  bits::is_pow2(x);
/// @endcode
/// @par Caveats
/// - Unsigned types only, as in `<bit>`: `bits::clz(-1)` does not compile; cast first.
/// - The std function does the work when the standard has it (`std::countl_zero` behind `bits::clz`, …); otherwise
///   a compiler builtin. Wider types are counted 64 bits at a time. `rotl`, `rotr` and `endian` are the std ones
///   themselves from C++20.
#include "cp/config.hpp"
#include "cp/traits/traits.hpp"
#include <cstdint>
#include <limits>
#include <type_traits>
#if defined(__cpp_lib_bitops) || defined(__cpp_lib_bit_cast) || defined(__cpp_lib_endian) || \
    defined(__cpp_lib_byteswap)  // cp:since c++20
#  include <bit>
#endif

namespace cp::detail {

/// @brief Types cp::bits accepts: unsigned integers of any width, not `bool` or character types.
template <class T>
inline constexpr bool is_bit_unsigned_v = is_integer_v<T> && !std::numeric_limits<T>::is_signed;

}  // namespace cp::detail

/// @namespace cp::bits
/// @brief Bit operations on unsigned integers.
namespace cp::bits {

/// @brief The bits of `from` as a `To` of the same size (both trivially copyable): `std::bit_cast`.
template <class To, class From>
constexpr auto cast(From const& from) noexcept -> std::enable_if_t<
    sizeof(To) == sizeof(From) && std::is_trivially_copyable_v<To> && std::is_trivially_copyable_v<From>, To> {
#if defined(__cpp_lib_bit_cast)  // cp:since c++20
    return std::bit_cast<To>(from);
#else
    return __builtin_bit_cast(To, from);
#endif
}

/// @brief Count of leading 0 bits; the width of `T` for 0.
template <class T>
constexpr auto clz(T x) noexcept -> int {
    static_assert(detail::is_bit_unsigned_v<T>, "cp::bits::clz requires an unsigned integer type");
    constexpr int n = detail::digits_v<T>;
    if constexpr (n > 64) {  // 64 bits at a time, from the top
        for (int s = n - 64; s >= 0; s -= 64) {
            auto const chunk = static_cast<std::uint64_t>(x >> s);
            if (chunk != 0) return n - 64 - s + clz(chunk);
        }
        return n;
    } else {
#if defined(__cpp_lib_bitops)  // cp:since c++20
        return std::countl_zero(x);
#else
        return x == 0 ? n : __builtin_clzll(x) - (64 - n);  // less the zero bits above a narrow T
#endif
    }
}

/// @brief Count of trailing 0 bits; the width of `T` for 0.
template <class T>
constexpr auto ctz(T x) noexcept -> int {
    static_assert(detail::is_bit_unsigned_v<T>, "cp::bits::ctz requires an unsigned integer type");
    constexpr int n = detail::digits_v<T>;
    if constexpr (n > 64) {  // 64 bits at a time, from the bottom
        for (int s = 0; s < n; s += 64) {
            auto const chunk = static_cast<std::uint64_t>(x >> s);
            if (chunk != 0) return s + ctz(chunk);
        }
        return n;
    } else {
#if defined(__cpp_lib_bitops)  // cp:since c++20
        return std::countr_zero(x);
#else
        return x == 0 ? n : __builtin_ctzll(x);
#endif
    }
}

/// @brief Count of leading 1 bits.
template <class T>
constexpr auto clo(T x) noexcept -> int {
    return clz(static_cast<T>(~x));
}
/// @brief Count of trailing 1 bits.
template <class T>
constexpr auto cto(T x) noexcept -> int {
    return ctz(static_cast<T>(~x));
}

/// @brief Count of 1 bits.
template <class T>
constexpr auto popcnt(T x) noexcept -> int {
    static_assert(detail::is_bit_unsigned_v<T>, "cp::bits::popcnt requires an unsigned integer type");
    if constexpr (detail::digits_v<T> > 64) {  // 64 bits at a time
        int count = 0;
        for (int s = 0; s < detail::digits_v<T>; s += 64) count += popcnt(static_cast<std::uint64_t>(x >> s));
        return count;
    } else {
#if defined(__cpp_lib_bitops)  // cp:since c++20
        return std::popcount(x);
#else
        return __builtin_popcountll(x);
#endif
    }
}

#if defined(__cpp_lib_bitops)  // cp:since c++20
using std::rotl;
using std::rotr;
#else
/// @brief `x` rotated left by `s` (any `int`; negative rotates right).
template <class T>
[[nodiscard]] constexpr auto rotl(T x, int s) noexcept -> T {
    static_assert(detail::is_bit_unsigned_v<T>, "cp::bits::rotl requires an unsigned integer type");
    constexpr int n = detail::digits_v<T>;
    int r           = s % n;
    if (r < 0) r += n;
    return r == 0 ? x : static_cast<T>((x << r) | (x >> (n - r)));
}
/// @brief `x` rotated right by `s` (any `int`; negative rotates left).
template <class T>
[[nodiscard]] constexpr auto rotr(T x, int s) noexcept -> T {
    return rotl(x, -(s % detail::digits_v<T>));
}
#endif

/// @brief Bits needed to hold `x`: `1 + floor(log2(x))`, 0 for 0.
template <class T>
constexpr auto width(T x) noexcept -> int {
    return detail::digits_v<T> - clz(x);
}
/// @brief True if `x` is a power of two.
template <class T>
constexpr auto is_pow2(T x) noexcept -> bool {
    static_assert(detail::is_bit_unsigned_v<T>, "cp::bits::is_pow2 requires an unsigned integer type");
    return x != 0 && (x & (x - 1u)) == 0;
}
/// @brief Largest power of two `<= x`; 0 for 0.
template <class T>
constexpr auto floor2(T x) noexcept -> T {
    return x == 0 ? T{0} : static_cast<T>(T{1} << (width(x) - 1));
}
/// @brief Smallest power of two `>= x`; 1 for 0.
/// @pre The result fits in `T`.
template <class T>
constexpr auto ceil2(T x) noexcept -> T {
    return x <= 1u ? T{1} : static_cast<T>(T{1} << width(static_cast<T>(x - 1u)));
}

/// @brief `x` with its bytes in reverse order (standard integer types up to 64 bits).
template <class T>
constexpr auto bswap(T x) noexcept -> T {
    static_assert(detail::is_integer_v<T> && std::is_integral_v<T> && sizeof(T) <= 8,
                  "cp::bits::bswap requires a standard integer type");
#if defined(__cpp_lib_byteswap)  // cp:since c++23
    return std::byteswap(x);
#else
    using U = std::make_unsigned_t<T>;
    auto u  = static_cast<U>(x);
    if constexpr (sizeof(T) == 1) {
        return x;
    } else if constexpr (sizeof(T) == 2) {
        return static_cast<T>(__builtin_bswap16(u));
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(__builtin_bswap32(u));
    } else {
        return static_cast<T>(__builtin_bswap64(u));
    }
#endif
}

#if defined(__cpp_lib_endian)  // cp:since c++20
using std::endian;
#else
/// @brief Byte order; `native` is `little` or `big`.
enum class endian { little = __ORDER_LITTLE_ENDIAN__, big = __ORDER_BIG_ENDIAN__, native = __BYTE_ORDER__ };
#endif

}  // namespace cp::bits
