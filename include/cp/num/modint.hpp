#pragma once
/// @file
/// @brief cp::num::modint<M> (modulus fixed at compile time, any M < 2⁶⁴) and cp::num::dyn_modint<Id> (modulus set at
/// run time, below 2³²): integers modulo M with `+ - * /`, pow() and inv().
/// @par Caveats
/// - Construction from an integer is explicit and maps any value, negatives included, into [0, M).
/// - `inv()` and `/` need gcd(x, M) = 1; with checks on, anything else is reported.
/// - dyn_modint has one modulus per `Id`, shared by all its values; set_mod() before use (998244353 until then).
///   Values made before a set_mod() keep their old residue.
/// - No ordering and no bitwise operators: they mean nothing modulo M.
#include "cp/config.hpp"
#include "cp/detail/integer.hpp"
#include "cp/detail/precondition.hpp"
#include "cp/num/cmp.hpp"
#include "cp/num/consts.hpp"
#include "cp/num/wide.hpp"
#include "cp/traits/traits.hpp"
#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

namespace cp::detail {

/// @brief The inverse of `a` modulo `m` by the extended Euclidean algorithm, computed in the signed type `S`
/// (holds ±m); `second` is false when gcd(a, m) ≠ 1.
template <class S, class V>
constexpr auto inv_mod_euclid(V a, V m) noexcept -> std::pair<V, bool> {
    S r0 = m, r1 = a, t0 = 0, t1 = 1;  // invariant: t·a ≡ r (mod m)
    while (r1 != S{}) {
        S const q = r0 / r1;
        S const r = r0 - q * r1;
        S const t = t0 - q * t1;
        r0 = r1, r1 = r, t0 = t1, t1 = t;
    }
    if (r0 != S{1}) return {V{}, false};
    return {static_cast<V>(t0 < S{} ? t0 + S{m} : t0), true};
}

/// @brief Barrett reduction for a run-time modulus below 2³²: `a·b mod m` by a multiply and a shift instead of a
/// division.
struct barrett {
    std::uint32_t m;   ///< the modulus
    std::uint64_t im;  ///< ⌈2⁶⁴ / m⌉, wrapped to 0 for m = 1

    /// @brief Precomputes ⌈2⁶⁴ / m⌉.
    constexpr explicit barrett(std::uint32_t mod) noexcept : m(mod), im(~std::uint64_t{0} / mod + 1) { }

    /// @brief `a·b mod m` for `a, b < m`. The estimate ⌊a·b·im / 2⁶⁴⌋ is the quotient or one more; a negative
    /// remainder is fixed by adding `m`.
    [[nodiscard]] constexpr auto mul(std::uint32_t a, std::uint32_t b) const noexcept -> std::uint32_t {
        std::uint64_t const z = std::uint64_t{a} * b;
        std::uint64_t const x = num::mul_wide(z, im).high;
        std::uint64_t const y = x * m;
        return static_cast<std::uint32_t>(z - y + (z < y ? m : 0));
    }
};

/// @brief The modulus of cp::num::modint: `M`, fixed at compile time.
template <std::uint64_t M>
struct static_modulus {
    static_assert(M >= 1, "cp::num::modint requires a modulus of at least 1");
    /// @brief The residue's type: 32 bits when M < 2³².
    using value_type = std::conditional_t<(M >> 32) == 0, std::uint32_t, std::uint64_t>;

    /// @brief The modulus, M.
    static constexpr auto mod() noexcept -> value_type { return M; }

protected:
    /// @brief `a·b mod M`. Below 2³² M is a constant `%` of a 64-bit product, which the compiler turns into a multiply
    /// and a shift; above, a 128-bit `%` (a library call: about half the speed).
    static constexpr auto m_mul(value_type a, value_type b) noexcept -> value_type {
        return static_cast<value_type>(num::widen_t<value_type>{a} * b % M);
    }
};

/// @brief The modulus of cp::num::dyn_modint<Id>: set at run time, one per `Id`.
template <int Id>
struct runtime_modulus {
    /// @brief The residue's type.
    using value_type = std::uint32_t;

    /// @brief The current modulus.
    static auto mod() noexcept -> std::uint32_t { return s_reducer.m; }
    /// @brief Sets the modulus for every value of this `Id`.
    /// @pre 1 ≤ m < 2³².
    template <class T>
    static void set_mod(T m) noexcept {
        static_assert(is_integer_v<T>, "cp::num::dyn_modint::set_mod requires an integer");
        detail::precondition(m != T{} && num::fits<std::uint32_t>(m), "dyn_modint::set_mod: modulus outside [1, 2^32)");
        s_reducer = barrett(static_cast<std::uint32_t>(m));
    }

protected:
    /// @brief `a·b mod mod()`, by Barrett reduction.
    static auto m_mul(std::uint32_t a, std::uint32_t b) noexcept -> std::uint32_t { return s_reducer.mul(a, b); }

private:
    inline static barrett s_reducer{num::consts::mod998};
};

/// @brief An integer modulo `Modulus::mod()`: cp::num::modint and cp::num::dyn_modint.
template <class Modulus>
class basic_modint : public Modulus {
    using V           = typename Modulus::value_type;
    using wide_signed = num::widen_t<std::make_signed_t<V>>;  // holds ±mod() and any value worth reading
    // Each operator takes a modint or an integer on either side (one side is a modint: these are hidden friends).
    template <class L, class R>
    using if_operands = std::enable_if_t<(std::is_same_v<L, basic_modint> || is_integer_v<L>) &&
                                             (std::is_same_v<R, basic_modint> || is_integer_v<R>),
                                         int>;

public:
    /// @brief Zero.
    constexpr basic_modint() noexcept = default;
    /// @brief `x mod mod()`, for any integer `x`.
    template <class T, std::enable_if_t<is_integer_v<T>, int> = 0>
    constexpr explicit basic_modint(T x) noexcept {
        auto const r = static_cast<V>(unsigned_abs(x) % Modulus::mod());
        m_v          = x < 0 && r != 0 ? static_cast<V>(Modulus::mod() - r) : r;
    }

    /// @brief The residue, in [0, mod()).
    [[nodiscard]] constexpr auto value() const noexcept -> V { return m_v; }

    /// @brief `x^e`, by squaring; a negative `e` raises the inverse.
    template <class E>
    [[nodiscard]] constexpr auto pow(E e) const noexcept -> basic_modint {
        static_assert(is_integer_v<E>, "cp::num::modint::pow requires an integer exponent");
        basic_modint base = e < 0 ? inv() : *this;
        basic_modint r(1);
        for (auto n = unsigned_abs(e); n != 0; n >>= 1) {
            if ((n & 1U) != 0) r *= base;
            base *= base;
        }
        return r;
    }
    /// @brief The `y` with `x·y = 1`.
    /// @pre gcd(value(), mod()) = 1.
    [[nodiscard]] constexpr auto inv() const noexcept -> basic_modint {
        auto const [y, ok] = inv_mod_euclid<wide_signed>(m_v, Modulus::mod());
        detail::precondition(ok, "modint::inv: the value and the modulus are not coprime");
        return m_raw(y);
    }

    /// @brief Unary plus: `x` itself.
    constexpr auto operator+() const noexcept -> basic_modint { return *this; }
    /// @brief `-x`.
    constexpr auto operator-() const noexcept -> basic_modint { return basic_modint{} - *this; }
    /// @brief Adds one.
    constexpr auto operator++() noexcept -> basic_modint& { return *this += 1; }
    /// @brief Subtracts one.
    constexpr auto operator--() noexcept -> basic_modint& { return *this -= 1; }
    /// @brief Adds one, returns the old value.
    constexpr auto operator++(int) noexcept -> basic_modint {
        basic_modint old  = *this;
        *this            += 1;
        return old;
    }
    /// @brief Subtracts one, returns the old value.
    constexpr auto operator--(int) noexcept -> basic_modint {
        basic_modint old  = *this;
        *this            -= 1;
        return old;
    }
    ///@{
    /// @brief Compound assignment; the right side may be an integer.
    template <class T>
    constexpr auto operator+=(T const& y) noexcept -> basic_modint& {
        return *this = *this + y;
    }
    template <class T>
    constexpr auto operator-=(T const& y) noexcept -> basic_modint& {
        return *this = *this - y;
    }
    template <class T>
    constexpr auto operator*=(T const& y) noexcept -> basic_modint& {
        return *this = *this * y;
    }
    template <class T>
    constexpr auto operator/=(T const& y) noexcept -> basic_modint& {
        return *this = *this / y;
    }
    ///@}

    ///@{
    /// @brief Arithmetic and comparison; either side may be an integer: `x * 2`, `1 - x`, `x == 0`.
    template <class L, class R, if_operands<L, R> = 0>
    friend constexpr auto operator+(L const& a, R const& b) noexcept -> basic_modint {
        return m_add(basic_modint(a), basic_modint(b));
    }
    template <class L, class R, if_operands<L, R> = 0>
    friend constexpr auto operator-(L const& a, R const& b) noexcept -> basic_modint {
        return m_sub(basic_modint(a), basic_modint(b));
    }
    template <class L, class R, if_operands<L, R> = 0>
    friend constexpr auto operator*(L const& a, R const& b) noexcept -> basic_modint {
        return m_raw(Modulus::m_mul(basic_modint(a).m_v, basic_modint(b).m_v));
    }
    template <class L, class R, if_operands<L, R> = 0>
    friend constexpr auto operator/(L const& a, R const& b) noexcept -> basic_modint {
        return basic_modint(a) * basic_modint(b).inv();
    }
    template <class L, class R, if_operands<L, R> = 0>
    friend constexpr auto operator==(L const& a, R const& b) noexcept -> bool {
        return basic_modint(a).m_v == basic_modint(b).m_v;
    }
    template <class L, class R, if_operands<L, R> = 0>
    friend constexpr auto operator!=(L const& a, R const& b) noexcept -> bool {
        return !(a == b);
    }
    ///@}

    /// @brief cp I/O: reads an integer and reduces it.
    template <class Reader>
    friend void cp_read(Reader& r, basic_modint& x) {
        wide_signed v{};
        if (r.read(v)) x = basic_modint(v);
    }
    /// @brief cp I/O: writes the residue.
    template <class Writer>
    friend void cp_write(Writer& w, basic_modint const& x) {
        w.write(x.m_v);
    }

private:
    V m_v{};

    /// @brief A modint holding `v`, which is already in [0, mod()).
    static constexpr auto m_raw(V v) noexcept -> basic_modint {
        basic_modint x;
        x.m_v = v;
        return x;
    }
    /// @brief The sum, less mod() once if it reached mod() or wrapped past the type's top.
    static constexpr auto m_add(basic_modint a, basic_modint b) noexcept -> basic_modint {
        auto const s = static_cast<V>(a.m_v + b.m_v);
        return m_raw(s >= Modulus::mod() || s < a.m_v ? static_cast<V>(s - Modulus::mod()) : s);
    }
    /// @brief The difference, plus mod() if it went below zero.
    static constexpr auto m_sub(basic_modint a, basic_modint b) noexcept -> basic_modint {
        auto const d = static_cast<V>(a.m_v - b.m_v);
        return m_raw(a.m_v < b.m_v ? static_cast<V>(d + Modulus::mod()) : d);
    }
};

}  // namespace cp::detail

namespace cp::num {

/// @brief An integer modulo `M`, fixed at compile time. Stored in 32 bits when M < 2³², else 64.
template <std::uint64_t M>
using modint = detail::basic_modint<detail::static_modulus<M>>;

/// @brief An integer modulo a run-time modulus below 2³², one modulus per `Id`: `dyn_modint<>::set_mod(m)`.
template <int Id = 0>
using dyn_modint = detail::basic_modint<detail::runtime_modulus<Id>>;

using modint998   = modint<consts::mod998>;    ///< modulo 998244353
using modint1e9_7 = modint<consts::mod1e9_7>;  ///< modulo 10⁹ + 7

}  // namespace cp::num

namespace std {

/// @brief Hash of a cp::num::modint or cp::num::dyn_modint: its residue's hash.
template <class Modulus>
struct hash<cp::detail::basic_modint<Modulus>> {
    /// @brief Hash of `x`.
    auto operator()(cp::detail::basic_modint<Modulus> const& x) const noexcept -> std::size_t {
        return std::hash<typename Modulus::value_type>{}(x.value());
    }
};

}  // namespace std
