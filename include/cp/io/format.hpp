#pragma once
/// @file
/// @brief Number formatting into a caller-sized buffer: decimal integers two digits at a time, integers in bases
/// 2–36, and floating point. Pure functions: no streams, no OS, no options state.
/// @par Caveats
/// - Integer functions write without a bound: the caller provides max_decimal_chars_v / max_int_chars_v bytes.
/// - Floats use `std::to_chars` when the library supports floating point in it; otherwise `snprintf`, which follows
///   the C locale's decimal point and prints the shortest form as `%.17g` (round-trips, but not always shortest).
#include "cp/config.hpp"
#include "cp/traits/traits.hpp"
#include "cp/bits/bits.hpp"
#include "cp/detail/integer.hpp"
#include "cp/detail/precondition.hpp"
#include <array>
#include <cstddef>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace cp::detail {

/// @brief 10^0 … 10^19.
inline constexpr auto pow10_u64 = [] {
    std::array<std::uint64_t, 20> t{};
    t[0] = 1;
    for (std::size_t i = 1; i < t.size(); ++i) t[i] = t[i - 1] * 10;
    return t;
}();

/// @brief `"00" "01" … "99"`: two characters per value.
inline constexpr auto digit_pairs = [] {
    std::array<char, 200> t{};
    for (std::size_t i = 0; i < 100; ++i) {
        t[2 * i]     = static_cast<char>('0' + i / 10);
        t[2 * i + 1] = static_cast<char>('0' + i % 10);
    }
    return t;
}();

/// @brief Number of decimal digits of `v` (1 for 0).
constexpr auto count_digits(std::uint64_t v) noexcept -> int {
    // floor(log10(v)) from the bit width (1233 / 4096 ≈ log10(2)), off by at most one; the table corrects it.
    auto t = static_cast<std::size_t>((bits::width(v | 1) * 1233) >> 12);
    return static_cast<int>(t) - ((v | 1) < pow10_u64[t] ? 1 : 0) + 1;  // v | 1: 0 has one digit
}

/// @brief Most characters format_int(p, v) writes for a `T`: digits plus a sign.
template <class T>
inline constexpr int max_decimal_chars_v = std::numeric_limits<unsigned_of_t<T>>::digits10 + 2;
/// @brief Most characters format_unsigned(p, v, base) writes for a `T`, in any base, plus a sign: binary digits + 1.
template <class T>
inline constexpr int max_int_chars_v = width_v<T> + 1;

/// @brief Writes exactly `n` decimal digits of `v`, zero-padded on the left, at `p`.
inline void write_digits(char* p, std::uint64_t v, int n) noexcept {
    char* q = p + n;
    for (; n >= 2; n -= 2, v /= 100) {
        q -= 2;
        std::memcpy(q, &digit_pairs[static_cast<std::size_t>(v % 100) * 2], 2);
    }
    if (n != 0) *--q = static_cast<char>('0' + v % 10);
}

/// @brief Writes `v` in decimal at `p`; returns the end.
template <class U>
auto format_unsigned(char* p, U v) noexcept -> char* {
    if constexpr (width_v<U> > 64) {
        constexpr std::uint64_t e19 = pow10_u64[19];
        if (v > U{std::numeric_limits<std::uint64_t>::max()}) {
            U q = v / U{e19};
            p   = format_unsigned(p, q);
            write_digits(p, static_cast<std::uint64_t>(v - q * U{e19}), 19);
            return p + 19;
        }
    }
    auto u = static_cast<std::uint64_t>(v);
    int n  = count_digits(u);
    write_digits(p, u, n);
    return p + n;
}

/// @brief Writes unsigned `u` in `base` (2–36) at `p`; digits above 9 are lower-case letters. Returns the end.
/// @pre `max_int_chars_v<U>` bytes are writable at `p`.
template <class U>
auto format_unsigned(char* p, U u, int base) noexcept -> char* {
    precondition(2 <= base && base <= 36, "cp::io: integer base outside 2..36");
    if (base == 10) return format_unsigned(p, u);
    constexpr std::string_view letters = "0123456789abcdefghijklmnopqrstuvwxyz";
    std::array<char, static_cast<std::size_t>(width_v<U>)> tmp{};
    auto i  = tmp.size();
    auto ub = static_cast<U>(base);
    do {
        tmp[--i] = letters[static_cast<std::size_t>(u % ub)];
        u        = u / ub;
    } while (u != U{});
    std::memcpy(p, tmp.data() + i, tmp.size() - i);
    return p + (tmp.size() - i);
}

/// @brief Writes integer `v` in decimal at `p`, with a `-` sign if negative; returns the end.
/// @pre max_decimal_chars_v<T> bytes are writable at `p`.
template <class T>
auto format_int(char* p, T v) noexcept -> char* {
    static_assert(is_integer_v<T>, "cp: format_int requires an integer type");
    if (v < 0) *p++ = '-';
    return format_unsigned(p, unsigned_abs(v));
}

/// @brief An upper bound on the characters format_float writes for `F` in `fmt` with `precision` (< 0: shortest).
template <class F>
constexpr auto max_float_chars(std::chars_format fmt, int precision) noexcept -> int {
    using L              = std::numeric_limits<F>;
    int const fraction   = precision >= 0 ? precision : L::max_digits10;
    int const scientific = 1 + 1 + 1 + fraction + 2 + 5 + 8;  // sign, digit, point, fraction, e+, exponent, slack
    if (fmt != std::chars_format::fixed) return scientific;
    int const integer_part = L::max_exponent10 + 1;
    // Shortest fixed prints every leading zero of a subnormal: down to 10^(min_exponent10 - digits10).
    int const fraction_part = precision >= 0 ? precision : L::max_digits10 - L::min_exponent10 + L::digits10 + 8;
    return 1 + integer_part + 1 + fraction_part;
}

/// @brief Writes `v` into [first, last) in `fmt` with `precision` digits (< 0: the shortest form that round-trips),
/// as std::to_chars does. Returns the end.
/// @pre `last - first` is at least max_float_chars<F>(fmt, precision).
template <class F>
auto format_float(char* first, char* last, F v, std::chars_format fmt, int precision) noexcept -> char* {
    static_assert(std::is_floating_point_v<F>, "cp: format_float requires a floating-point type");
#if defined(__cpp_lib_to_chars)  // floating-point to_chars: libstdc++ 11, libc++ 20
    auto r = precision < 0 ? std::to_chars(first, last, v, fmt) : std::to_chars(first, last, v, fmt, precision);
    precondition(r.ec == std::errc{}, "cp::io: float buffer too small");
    return r.ptr;
#else
    std::array<char, 8> spec{'%', '.', '*'};
    std::size_t i = 3;
    if constexpr (std::is_same_v<F, long double>) spec[i++] = 'L';
    spec[i] = fmt == std::chars_format::fixed      ? 'f'
            : fmt == std::chars_format::scientific ? 'e'
            : fmt == std::chars_format::hex        ? 'a'
                                                   : 'g';
    if (precision < 0) precision = std::numeric_limits<F>::max_digits10;
    auto size = static_cast<std::size_t>(last - first);
    int n     = std::snprintf(first, size, spec.data(), precision, v);  // NOLINT(cppcoreguidelines-pro-type-vararg)
    precondition(n >= 0 && static_cast<std::size_t>(n) < size, "cp::io: float buffer too small");
    return first + n;
#endif
}

}  // namespace cp::detail
