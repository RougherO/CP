#pragma once
/// @file
/// @brief cp::detail::double_uint<H> and double_int<H>: integers twice as wide as the unsigned half type `H`, built
/// from two `H` halves. `H` is `u64`, `unsigned __int128` or another double_uint, so the types nest to any
/// power-of-two width. They are the backport of cp::u128 / cp::i128 and the result of cp::widen_t above 128 bits.
/// @par Caveats
/// - Narrower integers convert in implicitly; converting to a narrower type, or between the signed and unsigned type
///   of one width, needs `static_cast`. Code written that way also compiles with `__int128`.
/// - `std::is_integral_v` and `std::make_unsigned_t` do not apply; `std::numeric_limits` and `std::hash` do.
/// - Signed overflow wraps.
/// - Each doubling roughly quadruples the cost of `*` and doubles the steps of `/` (binary long division above
///   the native width); the width is the user's call.
#include "cp/config.hpp"
#include "cp/compat/bit.hpp"
#include "cp/compat/meta.hpp"
#include "cp/compat/version.hpp"
#include "cp/detail/precondition.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <type_traits>
#if defined(__cpp_lib_constexpr_cmath) && __cpp_lib_constexpr_cmath >= 202202L  // cp:since c++23
#  include <cmath>
#endif
#if defined(__cpp_lib_three_way_comparison)  // cp:since c++20
#  include <compare>
#endif

namespace cp::detail {

#if CP_HAS_INT128
/// @brief The compiler's 128-bit integers, named once so `-pedantic` stays quiet.
__extension__ using builtin_i128 = __int128;
__extension__ using builtin_u128 = unsigned __int128;
#endif

template <class H>
class double_uint;
template <class H>
class double_int;

/// @brief The unsigned type of the same width: `std::make_unsigned` (which covers `__int128`), extended to cp's
/// double-width types.
template <class T>
struct unsigned_of : std::make_unsigned<T> { };
/// @cond
template <class H>
struct unsigned_of<double_uint<H>> {
    using type = double_uint<H>;
};
template <class H>
struct unsigned_of<double_int<H>> {
    using type = double_uint<H>;
};
/// @endcond
template <class T>
using unsigned_of_t = typename unsigned_of<T>::type;

/// @brief 2^n as `F`, exact (infinity when out of range).
template <class F>
constexpr auto pow2(int n) noexcept -> F {
#if defined(__cpp_lib_constexpr_cmath) && __cpp_lib_constexpr_cmath >= 202202L  // cp:since c++23
    return std::ldexp(F{1}, n);
#else
    F r = 1;
    for (; n >= 32; n -= 32) r *= static_cast<F>(std::uint64_t{1} << 32);
    return r * static_cast<F>(std::uint64_t{1} << n);
#endif
}

/// @brief Low half of `a * b` for a built-in unsigned `T`; the high half goes to `hi`. The double_uint version is a
/// hidden friend.
template <class T>
constexpr auto mul_full(T a, T b, T& hi) noexcept -> T {
#if CP_HAS_INT128
    if constexpr (width_v<T> == 64) {
        auto p = builtin_u128{a} * b;
        hi     = static_cast<T>(p >> 64);
        return static_cast<T>(p);
    }
#endif
    // Schoolbook on half-width pieces; each piece product fits in T.
    constexpr int h  = width_v<T> / 2;
    constexpr T mask = (T{1} << h) - 1;
    T a0 = a & mask, a1 = a >> h, b0 = b & mask, b1 = b >> h;
    T p00 = a0 * b0, p01 = a0 * b1, p10 = a1 * b0, p11 = a1 * b1;
    T mid = (p00 >> h) + (p01 & mask) + (p10 & mask);
    hi    = p11 + (p01 >> h) + (p10 >> h) + (mid >> h);
    return (mid << h) | (p00 & mask);
}

/// @brief Compound assignment, increment, decrement, unary `+ -` and (before C++20) the derived comparisons, written
/// once in terms of `D`'s `+ - * / % & | ^ << >> == <`.
template <class D>
class int_ops {
public:
    constexpr auto& operator+=(D b) noexcept { return self() = self() + b; }
    constexpr auto& operator-=(D b) noexcept { return self() = self() - b; }
    constexpr auto& operator*=(D b) noexcept { return self() = self() * b; }
    constexpr auto& operator/=(D b) noexcept { return self() = self() / b; }
    constexpr auto& operator%=(D b) noexcept { return self() = self() % b; }
    constexpr auto& operator&=(D b) noexcept { return self() = self() & b; }
    constexpr auto& operator|=(D b) noexcept { return self() = self() | b; }
    constexpr auto& operator^=(D b) noexcept { return self() = self() ^ b; }
    constexpr auto& operator<<=(int n) noexcept { return self() = self() << n; }
    constexpr auto& operator>>=(int n) noexcept { return self() = self() >> n; }
    constexpr auto& operator++() noexcept { return self() += 1; }
    constexpr auto& operator--() noexcept { return self() -= 1; }
    constexpr auto operator++(int) noexcept {
        D old = self();
        ++self();
        return old;
    }
    constexpr auto operator--(int) noexcept {
        D old = self();
        --self();
        return old;
    }

    constexpr auto operator+() const noexcept { return self(); }
    constexpr auto operator-() const noexcept { return D(0) - self(); }

#if !defined(__cpp_lib_three_way_comparison)
    friend constexpr auto operator!=(D a, D b) noexcept -> bool { return !(a == b); }
    friend constexpr auto operator>(D a, D b) noexcept -> bool { return b < a; }
    friend constexpr auto operator<=(D a, D b) noexcept -> bool { return !(b < a); }
    friend constexpr auto operator>=(D a, D b) noexcept -> bool { return !(a < b); }
#endif

private:
    friend D;
    constexpr int_ops() noexcept = default;

    constexpr auto self() noexcept -> D& { return static_cast<D&>(*this); }
    constexpr auto self() const noexcept -> D const& { return static_cast<D const&>(*this); }
};

/// @brief Unsigned integer of twice the width of the unsigned type `H`.
template <class H>
class double_uint : public int_ops<double_uint<H>> {
    static_assert(is_int_like_v<H> && !std::numeric_limits<H>::is_signed && width_v<H> >= 64,
                  "double_uint: H is u64, unsigned __int128 or a double_uint");
    static constexpr int half = width_v<H>;

public:
    constexpr double_uint() noexcept = default;

    /// @brief From a narrower integer; negative values wrap, as for built-in unsigned types.
    template <class I, std::enable_if_t<is_int_like_v<I> && (width_v<I> <= width_v<H>), int> = 0>
    constexpr double_uint(I v) noexcept  // NOLINT(google-explicit-constructor): behaves like an integer
        : m_lo(static_cast<H>(v)) {
        if constexpr (std::numeric_limits<I>::is_signed) m_hi = v < I{} ? ~H{} : H{};
    }
    /// @brief From a floating-point value, truncating; the value must be in [0, 2^width).
    template <class F, std::enable_if_t<std::is_floating_point_v<F>, int> = 0>
    explicit constexpr double_uint(F v) noexcept {
        precondition(v > F(-1) && v < pow2<F>(2 * half), "floating-point value out of range");
        if (v < pow2<F>(half)) {
            m_lo = static_cast<H>(v);
        } else {
            m_hi = static_cast<H>(v / pow2<F>(half));
            m_lo = static_cast<H>(v - static_cast<F>(m_hi) * pow2<F>(half));
        }
    }
    /// @brief Same bits, read as unsigned.
    explicit constexpr double_uint(double_int<H> v) noexcept;

    /// @brief Low bits, as `static_cast` between built-in integers gives.
    template <class I, std::enable_if_t<is_int_like_v<I> && (width_v<I> <= width_v<H>), int> = 0>
    explicit constexpr operator I() const noexcept {
        return static_cast<I>(m_lo);
    }
    explicit constexpr operator bool() const noexcept { return static_cast<bool>(m_lo | m_hi); }
    /// @brief Rounded to nearest.
    template <class F, std::enable_if_t<std::is_floating_point_v<F>, int> = 0>
    explicit constexpr operator F() const noexcept {
        if (m_hi == H{}) return static_cast<F>(m_lo);
        if constexpr (half >= std::numeric_limits<F>::digits + 2) {
            // The top `half` bits plus a sticky bit for the rest round exactly as the full value does.
            int shift   = half - countl_zero(m_hi);
            auto top    = static_cast<H>(*this >> shift);
            bool sticky = static_cast<bool>(*this & ((double_uint(1) << shift) - 1));
            return static_cast<F>(top | (sticky ? H{1} : H{})) * pow2<F>(shift);
        } else {
            static_assert(std::numeric_limits<F>::digits >= half, "both halves must convert exactly");
            return static_cast<F>(m_hi) * pow2<F>(half) + static_cast<F>(m_lo);  // one rounding
        }
    }

    friend constexpr auto operator+(double_uint a, double_uint b) noexcept -> double_uint {
        H lo = a.m_lo + b.m_lo;
        return from_halves(a.m_hi + b.m_hi + (lo < a.m_lo ? H{1} : H{}), lo);
    }
    friend constexpr auto operator-(double_uint a, double_uint b) noexcept -> double_uint {
        return from_halves(a.m_hi - b.m_hi - (a.m_lo < b.m_lo ? H{1} : H{}), a.m_lo - b.m_lo);
    }
    friend constexpr auto operator*(double_uint a, double_uint b) noexcept -> double_uint {
        H hi{};
        H lo = mul_full(a.m_lo, b.m_lo, hi);
        return from_halves(hi + a.m_lo * b.m_hi + a.m_hi * b.m_lo, lo);
    }
    /// @pre `b != 0`.
    friend constexpr auto operator/(double_uint a, double_uint b) noexcept -> double_uint {
        double_uint q, r;
        divmod(a, b, q, r);
        return q;
    }
    /// @pre `b != 0`.
    friend constexpr auto operator%(double_uint a, double_uint b) noexcept -> double_uint {
        double_uint q, r;
        divmod(a, b, q, r);
        return r;
    }

    friend constexpr auto operator&(double_uint a, double_uint b) noexcept -> double_uint {
        return from_halves(a.m_hi & b.m_hi, a.m_lo & b.m_lo);
    }
    friend constexpr auto operator|(double_uint a, double_uint b) noexcept -> double_uint {
        return from_halves(a.m_hi | b.m_hi, a.m_lo | b.m_lo);
    }
    friend constexpr auto operator^(double_uint a, double_uint b) noexcept -> double_uint {
        return from_halves(a.m_hi ^ b.m_hi, a.m_lo ^ b.m_lo);
    }
    constexpr auto operator~() const noexcept -> double_uint { return from_halves(~m_hi, ~m_lo); }
    /// @pre `0 <= n < width`.
    friend constexpr auto operator<<(double_uint a, int n) noexcept -> double_uint {
        precondition(0 <= n && n < 2 * half, "shift count out of range");
        if (n == 0) return a;
        if (n >= half) return from_halves(a.m_lo << (n - half), H{});
        return from_halves((a.m_hi << n) | (a.m_lo >> (half - n)), a.m_lo << n);
    }
    /// @pre `0 <= n < width`.
    friend constexpr auto operator>>(double_uint a, int n) noexcept -> double_uint {
        precondition(0 <= n && n < 2 * half, "shift count out of range");
        if (n == 0) return a;
        if (n >= half) return from_halves(H{}, a.m_hi >> (n - half));
        return from_halves(a.m_hi >> n, (a.m_lo >> n) | (a.m_hi << (half - n)));
    }

    friend constexpr auto operator==(double_uint a, double_uint b) noexcept -> bool {
        return a.m_hi == b.m_hi && a.m_lo == b.m_lo;
    }
#if defined(__cpp_lib_three_way_comparison)  // cp:since c++20
    friend constexpr auto operator<=>(double_uint a, double_uint b) noexcept -> std::strong_ordering {
        return a.m_hi != b.m_hi ? a.m_hi <=> b.m_hi : a.m_lo <=> b.m_lo;
    }
#else
    friend constexpr auto operator<(double_uint a, double_uint b) noexcept -> bool {
        return a.m_hi != b.m_hi ? a.m_hi < b.m_hi : a.m_lo < b.m_lo;
    }
#endif

    /// @brief Leading zero bits, as cp::countl_zero for built-in types.
    friend constexpr auto countl_zero(double_uint x) noexcept -> int {
        return x.m_hi != H{} ? countl_zero(x.m_hi) : half + countl_zero(x.m_lo);
    }
    /// @brief Low half of `a * b`; the high half goes to `hi`. Schoolbook on the four half products.
    friend constexpr auto mul_full(double_uint a, double_uint b, double_uint& hi) noexcept -> double_uint {
        H h00{}, h01{}, h10{}, h11{};
        H l00    = mul_full(a.m_lo, b.m_lo, h00);
        H l01    = mul_full(a.m_lo, b.m_hi, h01);
        H l10    = mul_full(a.m_hi, b.m_lo, h10);
        H l11    = mul_full(a.m_hi, b.m_hi, h11);
        auto mid = double_uint(h00) + double_uint(l01) + double_uint(l10);  // < 3 * 2^half: the carry fits
        hi       = from_halves(h11, l11) + double_uint(h01) + double_uint(h10) + double_uint(mid.m_hi);
        return from_halves(mid.m_lo, l00);
    }

private:
    template <class>
    friend class double_int;

    H m_lo{};
    H m_hi{};

    static constexpr auto from_halves(H hi, H lo) noexcept -> double_uint {
        double_uint r;
        r.m_hi = hi;
        r.m_lo = lo;
        return r;
    }
    /// @brief `(u1:u0) / v` with the remainder in `r`, for `u1 < v`: schoolbook division in two half-digit steps, each
    /// one native `H` division (Hacker's Delight `divlu`).
    static constexpr auto div_2by1(H u1, H u0, H v, H& r) noexcept -> H {
        constexpr int hh = half / 2;
        H const base = H{1} << hh, mask = base - 1;
        int s   = countl_zero(v);  // normalise: the divisor's top bit set keeps each digit estimate within 2
        v     <<= s;
        H vn1 = v >> hh, vn0 = v & mask;
        H un32 = s == 0 ? u1 : (u1 << s) | (u0 >> (half - s));
        H un10 = u0 << s;
        H un1 = un10 >> hh, un0 = un10 & mask;

        H q1 = un32 / vn1, rhat = un32 - q1 * vn1;
        while (q1 >= base || q1 * vn0 > ((rhat << hh) | un1)) {
            --q1;
            rhat += vn1;
            if (rhat >= base) break;
        }
        H un21 = (un32 << hh) + un1 - q1 * v;
        H q0   = un21 / vn1;
        rhat   = un21 - q0 * vn1;
        while (q0 >= base || q0 * vn0 > ((rhat << hh) | un0)) {
            --q0;
            rhat += vn1;
            if (rhat >= base) break;
        }
        r = ((un21 << hh) + un0 - q0 * v) >> s;
        return (q1 << hh) | q0;
    }
    /// @brief Quotient and remainder. A divisor that fits in `H` takes two div_2by1 steps; a wider one gets its
    /// quotient (which fits in `H`) estimated from the top halves and corrected once (Hacker's Delight `divu`).
    static constexpr void divmod(double_uint a, double_uint b, double_uint& q, double_uint& r) noexcept {
        precondition(b != 0, "division by zero");
        // NOLINTBEGIN(clang-analyzer-core.DivideZero): b != 0 is the precondition above
        if (b.m_hi == H{}) {
            if (a.m_hi == H{}) {
                q = a.m_lo / b.m_lo;
                r = a.m_lo % b.m_lo;
                return;
            }
            H rem{};
            H q_lo = div_2by1(a.m_hi % b.m_lo, a.m_lo, b.m_lo, rem);
            q      = from_halves(a.m_hi / b.m_lo, q_lo);
            r      = rem;
            return;
        }
        // NOLINTEND(clang-analyzer-core.DivideZero)
        if (a < b) {
            q = 0;
            r = a;
            return;
        }
        int n   = countl_zero(b.m_hi);
        H v1    = (b << n).m_hi;  // top bit set
        auto u1 = a >> 1;         // so u1.m_hi < v1
        H ignored{};
        H q1    = div_2by1(u1.m_hi, u1.m_lo, v1, ignored);
        auto qq = (double_uint(q1) << n) >> (half - 1);  // undo the normalisation and the halving: q or q + 1
        if (qq != 0) --qq;
        r = a - qq * b;
        if (r >= b) {
            ++qq;
            r -= b;
        }
        q = qq;
    }
};

/// @brief Signed two's-complement integer of twice the width of the unsigned type `H`.
template <class H>
class double_int : public int_ops<double_int<H>> {
    using bits_t              = double_uint<H>;
    static constexpr int half = width_v<H>;

public:
    constexpr double_int() noexcept = default;

    /// @brief From a narrower integer.
    template <class I, std::enable_if_t<is_int_like_v<I> && (width_v<I> <= width_v<H>), int> = 0>
    constexpr double_int(I v) noexcept  // NOLINT(google-explicit-constructor): behaves like an integer
        : m_bits(v) { }
    /// @brief From a floating-point value, truncating towards zero; the value must be in (-2^(width-1) - 1,
    /// 2^(width-1)).
    template <class F, std::enable_if_t<std::is_floating_point_v<F>, int> = 0>
    explicit constexpr double_int(F v) noexcept : m_bits(v < 0 ? -bits_t(-v) : bits_t(v)) {
        precondition(v < 0 ? !(v < -pow2<F>(2 * half - 1)) : v < pow2<F>(2 * half - 1),
                     "floating-point value out of range");
    }
    /// @brief Same bits, read as two's complement.
    explicit constexpr double_int(bits_t v) noexcept : m_bits(v) { }

    /// @brief Low bits, as `static_cast` between built-in integers gives.
    template <class I, std::enable_if_t<is_int_like_v<I> && (width_v<I> <= width_v<H>), int> = 0>
    explicit constexpr operator I() const noexcept {
        return static_cast<I>(m_bits.m_lo);
    }
    explicit constexpr operator bool() const noexcept { return static_cast<bool>(m_bits); }
    /// @brief Rounded to nearest.
    template <class F, std::enable_if_t<std::is_floating_point_v<F>, int> = 0>
    explicit constexpr operator F() const noexcept {
        return negative() ? -static_cast<F>(-m_bits) : static_cast<F>(m_bits);
    }

    friend constexpr auto operator+(double_int a, double_int b) noexcept -> double_int {
        return double_int(a.m_bits + b.m_bits);
    }
    friend constexpr auto operator-(double_int a, double_int b) noexcept -> double_int {
        return double_int(a.m_bits - b.m_bits);
    }
    friend constexpr auto operator*(double_int a, double_int b) noexcept -> double_int {
        return double_int(a.m_bits * b.m_bits);
    }
    /// @brief Truncates towards zero. @pre `b != 0`, and not `min / -1`.
    friend constexpr auto operator/(double_int a, double_int b) noexcept -> double_int {
        precondition(!(a == min() && b == -1), "division overflows");
        auto q = double_int(a.magnitude() / b.magnitude());
        return a.negative() != b.negative() ? -q : q;
    }
    /// @brief Takes the sign of `a`. @pre `b != 0`, and not `min % -1`.
    friend constexpr auto operator%(double_int a, double_int b) noexcept -> double_int {
        precondition(!(a == min() && b == -1), "division overflows");
        auto r = double_int(a.magnitude() % b.magnitude());
        return a.negative() ? -r : r;
    }

    friend constexpr auto operator&(double_int a, double_int b) noexcept -> double_int {
        return double_int(a.m_bits & b.m_bits);
    }
    friend constexpr auto operator|(double_int a, double_int b) noexcept -> double_int {
        return double_int(a.m_bits | b.m_bits);
    }
    friend constexpr auto operator^(double_int a, double_int b) noexcept -> double_int {
        return double_int(a.m_bits ^ b.m_bits);
    }
    constexpr auto operator~() const noexcept -> double_int { return double_int(~m_bits); }
    /// @pre `0 <= n < width`.
    friend constexpr auto operator<<(double_int a, int n) noexcept -> double_int { return double_int(a.m_bits << n); }
    /// @brief Arithmetic shift (copies the sign bit). @pre `0 <= n < width`.
    friend constexpr auto operator>>(double_int a, int n) noexcept -> double_int {
        return a.negative() ? double_int(~(~a.m_bits >> n)) : double_int(a.m_bits >> n);
    }

    friend constexpr auto operator==(double_int a, double_int b) noexcept -> bool { return a.m_bits == b.m_bits; }
#if defined(__cpp_lib_three_way_comparison)  // cp:since c++20
    friend constexpr auto operator<=>(double_int a, double_int b) noexcept -> std::strong_ordering {
        return a.biased() <=> b.biased();
    }
#else
    friend constexpr auto operator<(double_int a, double_int b) noexcept -> bool { return a.biased() < b.biased(); }
#endif

private:
    friend class double_uint<H>;

    bits_t m_bits;

    static constexpr auto sign_bit() noexcept -> bits_t { return bits_t::from_halves(H{1} << (half - 1), H{}); }
    static constexpr auto min() noexcept -> double_int { return double_int(sign_bit()); }
    constexpr auto negative() const noexcept -> bool { return static_cast<bool>(m_bits & sign_bit()); }
    /// @brief |x| as unsigned; min() maps to 2^(width-1).
    constexpr auto magnitude() const noexcept -> bits_t { return negative() ? -m_bits : m_bits; }
    /// @brief Sign bit flipped, so unsigned order is signed order.
    constexpr auto biased() const noexcept -> bits_t { return m_bits ^ sign_bit(); }
};

template <class H>
constexpr double_uint<H>::double_uint(double_int<H> v) noexcept : double_uint(v.m_bits) { }

/// @brief `std::numeric_limits` members shared by every double-width type.
template <bool Signed, int Width>
struct double_limits_base {
    static constexpr bool is_specialized                = true;
    static constexpr bool is_signed                     = Signed;
    static constexpr bool is_integer                    = true;
    static constexpr bool is_exact                      = true;
    static constexpr bool has_infinity                  = false;
    static constexpr bool has_quiet_NaN                 = false;  // NOLINT(readability-identifier-naming): std name
    static constexpr bool has_signaling_NaN             = false;  // NOLINT(readability-identifier-naming): std name
    static constexpr std::float_round_style round_style = std::round_toward_zero;
    static constexpr bool is_iec559                     = false;
    static constexpr bool is_bounded                    = true;
    static constexpr bool is_modulo                     = !Signed;
    static constexpr int digits                         = Signed ? Width - 1 : Width;
    static constexpr int digits10                       = digits * 30103 / 100000;  // floor(digits * log10(2))
    static constexpr int max_digits10                   = 0;
    static constexpr int radix                          = 2;
    static constexpr int min_exponent                   = 0;
    static constexpr int min_exponent10                 = 0;
    static constexpr int max_exponent                   = 0;
    static constexpr int max_exponent10                 = 0;
    static constexpr bool traps                         = true;
    static constexpr bool tinyness_before               = false;
};

}  // namespace cp::detail

namespace std {

/// @brief Limits of cp::detail::double_uint.
template <class H>
struct numeric_limits<cp::detail::double_uint<H>> : cp::detail::double_limits_base<false, 2 * cp::detail::width_v<H>> {
    using type = cp::detail::double_uint<H>;
    static constexpr auto min() noexcept { return type(0); }
    static constexpr auto lowest() noexcept { return type(0); }
    static constexpr auto max() noexcept { return ~type(0); }
    static constexpr auto epsilon() noexcept { return type(0); }
    static constexpr auto round_error() noexcept { return type(0); }
    static constexpr auto infinity() noexcept { return type(0); }
    static constexpr auto quiet_NaN() noexcept { return type(0); }      // NOLINT(readability-identifier-naming)
    static constexpr auto signaling_NaN() noexcept { return type(0); }  // NOLINT(readability-identifier-naming)
    static constexpr auto denorm_min() noexcept { return type(0); }
};

/// @brief Limits of cp::detail::double_int.
template <class H>
struct numeric_limits<cp::detail::double_int<H>> : cp::detail::double_limits_base<true, 2 * cp::detail::width_v<H>> {
    using type = cp::detail::double_int<H>;
    static constexpr auto max() noexcept { return type(~cp::detail::double_uint<H>(0) >> 1); }
    static constexpr auto min() noexcept { return ~max(); }
    static constexpr auto lowest() noexcept { return min(); }
    static constexpr auto epsilon() noexcept { return type(0); }
    static constexpr auto round_error() noexcept { return type(0); }
    static constexpr auto infinity() noexcept { return type(0); }
    static constexpr auto quiet_NaN() noexcept { return type(0); }      // NOLINT(readability-identifier-naming)
    static constexpr auto signaling_NaN() noexcept { return type(0); }  // NOLINT(readability-identifier-naming)
    static constexpr auto denorm_min() noexcept { return type(0); }
};

/// @brief Hash of cp::detail::double_uint, from the hashes of its halves.
template <class H>
struct hash<cp::detail::double_uint<H>> {
    auto operator()(cp::detail::double_uint<H> x) const noexcept -> std::size_t {
        constexpr int half = cp::detail::width_v<H>;
        std::size_t lo = std::hash<H>{}(static_cast<H>(x)), hi = std::hash<H>{}(static_cast<H>(x >> half));
        return lo ^ (hi * 0x9e37'79b9'7f4a'7c15);
    }
};

/// @brief Hash of cp::detail::double_int.
template <class H>
struct hash<cp::detail::double_int<H>> {
    auto operator()(cp::detail::double_int<H> x) const noexcept -> std::size_t {
        return std::hash<cp::detail::double_uint<H>>{}(cp::detail::double_uint<H>(x));
    }
};

}  // namespace std
