#pragma once
/// @file
/// @brief Number parsing over a padded byte buffer: decimal integers, integers in bases 2–36, and floating point.
/// Pure functions: no streams, no OS, no options state.
/// @par Caveats
/// - Precondition for every parser: the token ends with a byte that is not part of a number (whitespace or the
///   reader's `'\0'` sentinel). cp's reader guarantees it; the functions do no bounds checks.
/// - Overflow is always detected, at no cost for numbers of up to 19 digits; the value is then unspecified.
/// - Integers accept one leading `+` or `-` (`-` only for signed types), as `std::cin` does but `std::from_chars`
///   does not. Floats accept a leading `+`.
/// - Floats use `std::from_chars` when the library supports floating point in it; otherwise `strtod`, which follows
///   the C locale's decimal point and also accepts hexadecimal floats.
#include "cp/config.hpp"
#include "cp/traits/traits.hpp"
#include "cp/detail/integer.hpp"
#include "cp/detail/precondition.hpp"
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cerrno>  // IWYU pragma: keep (the strtod fallback without floating-point from_chars)
#include <cstdlib>
#include <limits>
#include <system_error>
#include <type_traits>

namespace cp::detail {

/// @brief How a parse ended.
// Word-sized on purpose: with a 1-byte status GCC returns parse_result through the stack and stalls every call.
enum class parse_status : std::size_t {  // NOLINT(performance-enum-size)
    ok,                                  ///< A number was read.
    invalid,                             ///< No number at the cursor (`end` is the cursor).
    overflow,  ///< The number does not fit; the whole token was consumed and the value is unspecified.
};

/// @brief The byte after the number, and how the parse ended.
struct parse_result {
    /// @brief The byte after the number.
    char const* end;
    /// @brief How the parse ended.
    parse_status status;
};

/// @brief True for ASCII whitespace and control bytes, including `'\0'` (`c <= ' '`; no locale, no signed-char UB).
constexpr auto is_space(char c) noexcept -> bool { return static_cast<unsigned char>(c) <= ' '; }

/// @brief The first whitespace byte at or after `p` (the reader's `'\0'` sentinel at the latest).
constexpr auto token_end(char const* p) noexcept -> char const* {
    while (!is_space(*p)) ++p;
    return p;
}

/// @brief Reads the digits in `base` at `p` into `out` (unsigned `U`), one at a time, with overflow checks.
template <class U>
auto parse_digits_checked(char const* p, U& out, unsigned base) noexcept -> parse_result {
    char const* const first = p;
    U const ub = static_cast<U>(base), limit = std::numeric_limits<U>::max() / ub,
            last = std::numeric_limits<U>::max() % ub;
    U r{};
    bool overflow = false;
    for (unsigned d = 0; (d = digit_value(*p)) < base; ++p)
        overflow |= !append_digit(r, ub, static_cast<U>(d), limit, last);
    out = r;
    if (p == first) return {first, parse_status::invalid};
    return {p, overflow ? parse_status::overflow : parse_status::ok};
}

/// @brief Reads the decimal digits at `p` into `out` (unsigned `U` of at least 64 bits).
/// A plain digit loop in 64 bits: its exit branch is predicted, so the CPU starts on the next number before this
/// one is done. (8-digits-at-a-time SWAR was measured slower: it turns that prediction into a data dependency.)
/// Numbers of more than 19 digits, which a u64 may not hold, are read again with overflow checks.
template <class U>
auto parse_digits(char const* p, U& out) noexcept -> parse_result {
    static_assert(!std::numeric_limits<U>::is_signed && width_v<U> >= 64);
    char const* const first = p;
    std::uint64_t r         = 0;
    for (unsigned d = 0; (d = static_cast<unsigned char>(*p) - unsigned{'0'}) < 10; ++p) r = r * 10 + d;
    auto const n = p - first;
    if (CP_UNLIKELY(n > std::numeric_limits<std::uint64_t>::digits10)) return parse_digits_checked(first, out, 10);
    out = r;
    return {p, n == 0 ? parse_status::invalid : parse_status::ok};
}

/// @brief The unsigned type integers of type `T` are accumulated in: at least 64 bits.
template <class T>
using accumulator_t = std::conditional_t<(width_v<T> <= 64), std::uint64_t, unsigned_of_t<T>>;

/// @brief Parses an integer in `base` (2–36) at `p`: an optional sign, then digits; above 9 they are letters, either
/// case. Base 10 takes the fast loop.
template <class T>
auto parse_int(char const* p, T& out, int base = 10) noexcept -> parse_result {
    static_assert(is_integer_v<T>, "cp: integer parsing requires an integer type");
    precondition(2 <= base && base <= 36, "cp::io: integer base outside 2..36");
    using U                 = accumulator_t<T>;
    char const* const first = p;
    bool const negative     = *p == '-';
    if constexpr (!std::numeric_limits<T>::is_signed) {
        if (negative) return {first, parse_status::invalid};
    }
    p += (negative || *p == '+') ? 1 : 0;  // branch-free: signs are unpredictable in mixed input
    U magnitude{};
    parse_result res =
        base == 10 ? parse_digits(p, magnitude) : parse_digits_checked(p, magnitude, static_cast<unsigned>(base));
    if (res.status == parse_status::invalid) return {first, parse_status::invalid};
    U const limit = static_cast<U>(std::numeric_limits<T>::max()) + U{negative ? 1U : 0U};
    if (magnitude > limit) res.status = parse_status::overflow;
    out = static_cast<T>(static_cast<unsigned_of_t<T>>(negative ? U{} - magnitude : magnitude));
    return res;
}

/// @brief Parses a floating-point number at `p`: optional `+`/`-`, then what `std::from_chars` accepts in `fmt`
/// (`inf`, `nan` included). On overflow the value is unspecified.
template <class F>
auto parse_float(char const* p, F& out, std::chars_format fmt = std::chars_format::general) noexcept -> parse_result {
    static_assert(std::is_floating_point_v<F>, "cp: parse_float requires a floating-point type");
    char const* const first = p;
    if (*p == '+' && p[1] != '-') ++p;
#if defined(__cpp_lib_to_chars)  // floating-point from_chars: libstdc++ 11, libc++ 20
    auto [end, ec] = std::from_chars(p, token_end(p), out, fmt);
    if (ec == std::errc::invalid_argument) return {first, parse_status::invalid};
    return {end, ec == std::errc{} ? parse_status::ok : parse_status::overflow};
#else
    static_cast<void>(fmt);
    char* end = nullptr;
    errno     = 0;
    if constexpr (std::is_same_v<F, float>) {
        out = std::strtof(p, &end);
    } else if constexpr (std::is_same_v<F, double>) {
        out = std::strtod(p, &end);
    } else {
        out = std::strtold(p, &end);
    }
    if (end == p || is_space(*p)) return {first, parse_status::invalid};  // strtod skips leading whitespace
    return {end, errno == ERANGE ? parse_status::overflow : parse_status::ok};
#endif
}

}  // namespace cp::detail
