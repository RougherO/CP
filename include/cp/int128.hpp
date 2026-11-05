#pragma once
/// @file
/// @brief cp::i128 and cp::u128: `__int128` where the compiler has it, otherwise cp::detail::double_int<u64> /
/// double_uint<u64>; `_i128` / `_u128` literals for constants wider than 64 bits.
/// @par Caveats
/// - The backport converts built-in integers in implicitly but back only explicitly: write `static_cast<u64>(x)`,
///   which works for `__int128` too. `i128` and `u128` do not mix without a cast. See cp/detail/double_int.hpp.
/// - Signed overflow wraps in the backport and is undefined for `__int128`; rely on neither.
/// - Printing and parsing come with cp's I/O module; standard streams cannot print `__int128`.
/// - Literals accept decimal, `0x`, `0b`, octal and `'` separators; a value that does not fit is a compile error.
///   `-170141183460469231731687303715884105728_i128` does not fit (the minus sign applies afterwards); use
///   `std::numeric_limits<cp::i128>::min()`.
#include "cp/config.hpp"
#include "cp/detail/double_int.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace cp::detail {

/// @brief cp::u128 when the compiler has no `__int128`.
using u128_backport = double_uint<std::uint64_t>;
/// @brief cp::i128 when the compiler has no `__int128`.
using i128_backport = double_int<std::uint64_t>;

}  // namespace cp::detail

namespace cp {

#if CP_HAS_INT128
/// @brief Signed 128-bit integer.
using i128 = detail::builtin_i128;
/// @brief Unsigned 128-bit integer.
using u128 = detail::builtin_u128;
#else
using i128 = detail::i128_backport;
using u128 = detail::u128_backport;
#endif
namespace detail {

/// @brief Not constexpr: reaching it while evaluating a literal makes the literal a compile error.
inline void int128_literal_out_of_range() noexcept { }

/// @brief The value of the integer literal `Cs...`, which must not exceed `max`.
template <class U, char... Cs>
constexpr auto parse_int128_literal(U max) noexcept -> U {
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
        int digit_value = c <= '9' ? c - '0' : c <= 'F' ? c - 'A' + 10 : c - 'a' + 10;
        auto digit      = static_cast<unsigned>(digit_value);
        if (value > limit || (value == limit && digit > last)) int128_literal_out_of_range();
        value = value * base + digit;
    }
    return value;
}

}  // namespace detail

inline namespace literals {

/// @brief A cp::u128 constant; a value above 2^128 - 1 does not compile.
template <char... Cs>
constexpr auto operator""_u128() noexcept {
    constexpr auto value = detail::parse_int128_literal<u128, Cs...>(~u128{0});
    return value;
}
/// @brief A non-negative cp::i128 constant; a value above 2^127 - 1 does not compile.
template <char... Cs>
constexpr auto operator""_i128() noexcept {
    constexpr auto value = static_cast<i128>(detail::parse_int128_literal<u128, Cs...>(~u128{0} >> 1));
    return value;
}

}  // namespace literals

}  // namespace cp
