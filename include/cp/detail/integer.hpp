#pragma once
/// @file
/// @brief Integer helpers shared by cp's I/O and number types: the magnitude of a signed value, digit values,
/// appending a digit with overflow detection, and the integer literals (`_i128`, `_i64`, ...).
#include "cp/config.hpp"
#include "cp/traits/traits.hpp"
#include <array>
#include <cstddef>
#include <limits>
#include <type_traits>

namespace cp::detail {

/// @brief |x| as the unsigned type of the same width: right for every value, the most negative one included, where
/// `-x` overflows.
template <class T>
constexpr auto unsigned_abs(T x) noexcept -> unsigned_of_t<T> {
    using U = unsigned_of_t<T>;
    return x < 0 ? static_cast<U>(U{} - static_cast<U>(x)) : static_cast<U>(x);
}

/// @brief `value = value * base + digit`, unless that exceeds the maximum `max`, given as `limit = max / base` and
/// `last = max % base`; then `value` is unchanged and the result is false. One comparison per digit, no division.
template <class U>
constexpr auto append_digit(U& value, U base, U digit, U limit, U last) noexcept -> bool {
    if (value > limit || (value == limit && digit > last)) return false;
    value = value * base + digit;
    return true;
}

/// @brief Value of digit `c` in bases up to 36 (either case), or 36 or more if `c` is not a digit.
constexpr auto digit_value(char c) noexcept -> unsigned {
    unsigned u = static_cast<unsigned char>(c);
    if (u - '0' < 10U) return u - '0';
    u |= 0x20U;  // lower case
    return u - 'a' < 26U ? u - 'a' + 10 : 36;
}

/// @brief Not constexpr: reaching it while evaluating a literal makes the literal a compile error.
inline void int_literal_out_of_range() noexcept { }

/// @brief The value of the integer literal `Cs...`, which must not exceed `max`.
template <class U, char... Cs>
constexpr auto parse_int_literal(U max) noexcept -> U {
    constexpr std::array<char, sizeof...(Cs)> s{Cs...};
    std::size_t i = 0;
    unsigned base = 10;
    if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        base = 16;
        i    = 2;
    } else if (s.size() > 2 && s[0] == '0' && (s[1] == 'b' || s[1] == 'B')) {
        base = 2;
        i    = 2;
    } else if (s.size() > 1 && s[0] == '0') {
        base = 8;
        i    = 1;
    }
    U value = 0, limit = max / base, last = max % base;
    for (; i < s.size(); ++i) {
        char c = s[i];
        if (c == '\'') continue;
        if (!append_digit(value, static_cast<U>(base), static_cast<U>(digit_value(c)), limit, last)) {
            int_literal_out_of_range();
        }
    }
    return value;
}

/// @brief The integer literal `Cs...` as a `T`; above `T`'s maximum it does not compile (the body of cp's `_i64`,
/// `_u128`, … literals).
template <class T, char... Cs>
constexpr auto int_literal() noexcept -> T {
    using U              = std::make_unsigned_t<T>;
    constexpr auto value = static_cast<T>(parse_int_literal<U, Cs...>(static_cast<U>(std::numeric_limits<T>::max())));
    return value;
}

}  // namespace cp::detail
