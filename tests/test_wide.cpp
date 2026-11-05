#include "cp/wide.hpp"
#include <array>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <functional>
#include <limits>
#include <random>
#include <type_traits>
#include <vector>

namespace {

using u64 = std::uint64_t;
using cp::i128;
using cp::u128;
using u256 = cp::widen_t<u128>;
using i256 = cp::widen_t<i128>;
using u512 = cp::widen_t<u256>;
// The same widths built from the other half types, as an independent implementation to compare against.
using u256_alt = cp::detail::double_uint<cp::detail::u128_backport>;
using i256_alt = cp::detail::double_int<cp::detail::u128_backport>;

template <class T>
constexpr int width_v = cp::detail::width_v<T>;

template <class T>
using unsigned_t = cp::detail::unsigned_of_t<T>;

// Random value with a random bit length, so carries and every division path run.
template <class T>
auto random_value(std::mt19937_64& rng) {
    using U = unsigned_t<T>;
    U x{};
    for (int i = 0; i < width_v<T>; i += 64) x = width_v<T> > 64 ? U((x << 64) | U(rng())) : U(rng());
    int len = static_cast<int>(rng() % width_v<T>) + 1;
    if (len < width_v<T>) x &= static_cast<U>((U{1} << len) - 1U);
    return static_cast<T>(x);
}

template <class T>
auto values() {
    using L = std::numeric_limits<T>;
    std::vector<T> v{T{}, T{1}, static_cast<T>(2), L::max(), static_cast<T>(L::max() - 1), L::min()};
    if constexpr (std::numeric_limits<T>::is_signed) v.push_back(static_cast<T>(-1));
    std::mt19937_64 rng(7);
    for (int k = 0; k < 40; ++k) v.push_back(random_value<T>(rng));
    return v;
}

template <class T, std::size_t N = width_v<T> / 64>
auto to_words(T x) {
    using U = unsigned_t<T>;
    auto u  = static_cast<U>(x);
    std::array<u64, N> w{};
    for (std::size_t i = 0; i < N; ++i) w[i] = static_cast<u64>(u >> static_cast<int>(64 * i));
    return w;
}
template <class T, std::size_t N>
auto from_words(std::array<u64, N> const& w) {
    using U = unsigned_t<T>;
    U u{};
    for (std::size_t i = N; i-- > 0;) u = (u << 64) | U(w[i]);
    return static_cast<T>(u);
}

}  // namespace

TEMPLATE_TEST_CASE("op_wide(a, b).widen() equals the operation on widen_t", "[wide]", std::uint8_t, std::int8_t,
                   std::uint16_t, std::int16_t, std::uint32_t, std::int32_t, u64, std::int64_t, u128, i128) {
    using T = TestType;
    using W = cp::widen_t<T>;
    STATIC_REQUIRE(width_v<W> == 2 * width_v<T>);
    STATIC_REQUIRE(std::numeric_limits<W>::is_signed == std::numeric_limits<T>::is_signed);
    auto fits = [](W x) { return static_cast<W>(static_cast<T>(x)) == x; };
    auto vs   = values<T>();
    for (T a : vs) {
        for (T b : vs) {
            auto add = cp::add_wide(a, b), sub = cp::sub_wide(a, b), mul = cp::mul_wide(a, b);
            REQUIRE(add.widen() == static_cast<W>(W(a) + W(b)));
            REQUIRE(sub.widen() == static_cast<W>(W(a) - W(b)));
            REQUIRE(mul.widen() == static_cast<W>(W(a) * W(b)));
            REQUIRE(add.low == static_cast<T>(add.widen()));
            REQUIRE(mul.low == static_cast<T>(mul.widen()));
            if constexpr (std::numeric_limits<T>::is_signed) {
                REQUIRE(add.overflow() == !fits(add.widen()));
                REQUIRE(sub.overflow() == !fits(sub.widen()));
            } else {
                REQUIRE(add.overflow() == (add.high != 0));
                REQUIRE(sub.overflow() == (a < b));
            }
            REQUIRE(mul.overflow() == !fits(mul.widen()));
            if constexpr (width_v<T> >= 64) {  // the portable versions run only on MSVC and for cp's types
                auto padd = cp::detail::add_wide_portable(a, b), psub = cp::detail::sub_wide_portable(a, b);
                REQUIRE((padd.low == add.low && padd.high == add.high));
                REQUIRE((psub.low == sub.low && psub.high == sub.high));
            }
        }
    }
}

TEST_CASE("256-bit types match the implementation built on the u128 backport", "[wide]") {
    auto vs = values<u256>();
    for (u256 a : vs) {
        u256_alt x = from_words<u256_alt>(to_words(a));
        for (int n : {0, 1, 63, 64, 127, 128, 200, 255}) {
            REQUIRE(to_words(a << n) == to_words(x << n));
            REQUIRE(to_words(a >> n) == to_words(x >> n));
            auto sa = static_cast<i256>(a);
            REQUIRE(to_words(sa >> n) == to_words(static_cast<i256_alt>(x) >> n));
        }
        for (u256 b : vs) {
            u256_alt y = from_words<u256_alt>(to_words(b));
            REQUIRE(to_words(a + b) == to_words(x + y));
            REQUIRE(to_words(a - b) == to_words(x - y));
            REQUIRE(to_words(a * b) == to_words(x * y));
            REQUIRE((a < b) == (x < y));
            if (b != 0) {
                REQUIRE(to_words(a / b) == to_words(x / y));
                REQUIRE(to_words(a % b) == to_words(x % y));
                REQUIRE(a / b * b + a % b == a);
                REQUIRE(a % b < b);
            }
            auto sa = static_cast<i256>(a), sb = static_cast<i256>(b);
            REQUIRE((sa < sb) == (static_cast<i256_alt>(x) < static_cast<i256_alt>(y)));
            if (sb != 0 && !(sa == std::numeric_limits<i256>::min() && sb == -1)) {
                REQUIRE(to_words(sa / sb) == to_words(static_cast<i256_alt>(x) / static_cast<i256_alt>(y)));
                REQUIRE(to_words(sa % sb) == to_words(static_cast<i256_alt>(x) % static_cast<i256_alt>(y)));
            }
        }
    }
}

TEST_CASE("wider types nest and keep integer behaviour", "[wide]") {
    STATIC_REQUIRE(std::is_same_v<cp::widen_t<std::uint32_t>, u64>);
    STATIC_REQUIRE(std::is_same_v<cp::widen_t<u64>, u128>);
    STATIC_REQUIRE(std::is_same_v<cp::widen_t<std::int64_t>, i128>);
    STATIC_REQUIRE(width_v<u512> == 512);
    STATIC_REQUIRE(std::numeric_limits<u256>::digits == 256 && std::numeric_limits<u256>::digits10 == 77);
    STATIC_REQUIRE(std::numeric_limits<i256>::digits == 255);

    // 128 x 128 -> 256 and 256 x 256 -> 512, at compile time.
    constexpr auto max128 = std::numeric_limits<u128>::max();
    STATIC_REQUIRE(cp::mul_wide(max128, max128).high == max128 - 1);
    STATIC_REQUIRE(cp::mul_wide(max128, max128).low == 1);
    constexpr auto p = cp::mul_wide(u256(max128) << 100, u256(3) << 150);  // 2^378 scale: overflows 256 bits
    STATIC_REQUIRE(p.overflow());
    STATIC_REQUIRE(p.widen() == u512(u256(max128) << 100) * u512(u256(3) << 150));
    STATIC_REQUIRE(p.widen() / (u512(3) << 150) == u512(max128) << 100);

    // Narrower values convert in implicitly; narrowing needs a cast.
    u256 x  = max128;
    x      += u128{1};
    REQUIRE(x == u256(1) << 128);
    REQUIRE(static_cast<u64>(x >> 128) == 1);
    STATIC_REQUIRE(!std::is_convertible_v<u256, u128>);
    STATIC_REQUIRE(!std::is_convertible_v<i256, u256>);

    // Floating point: rounded to nearest, ties to even.
    u256 big = u256(1) << 200;
    REQUIRE(static_cast<double>(big) == 0x1p200);
    REQUIRE(static_cast<double>(big + (u256(1) << 147)) == 0x1p200);  // exactly half an ulp: even stays
    REQUIRE(static_cast<double>(big + (u256(1) << 147) + 1) == 0x1p200 + 0x1p148);
    REQUIRE(static_cast<double>(-static_cast<i256>(big)) == -0x1p200);
    REQUIRE(u256(0x1p200) == big);
    REQUIRE(std::hash<u256>{}(big) != std::hash<u256>{}(big + 1));
}

TEST_CASE("wide results support each overflow policy", "[wide]") {
    auto [lo, hi] = cp::mul_wide(~u64{0}, ~u64{0});
    REQUIRE(lo == 1);
    REQUIRE(hi == ~u64{0} - 1);

    // Saturation: high's sign says which way the result left the range.
    auto saturating_add = [](std::int64_t a, std::int64_t b) {
        auto r = cp::add_wide(a, b);
        if (!r.overflow()) return r.low;
        return r.high < 0 ? std::numeric_limits<std::int64_t>::min() : std::numeric_limits<std::int64_t>::max();
    };
    REQUIRE(saturating_add(INT64_MAX, 1) == INT64_MAX);
    REQUIRE(saturating_add(INT64_MIN, -1) == INT64_MIN);
    REQUIRE(saturating_add(5, -7) == -2);

    STATIC_REQUIRE(cp::sub_wide(1U, 2U).high == ~0U);
    STATIC_REQUIRE(cp::sub_wide(1U, 2U).widen() == u64{1} - 2);
    STATIC_REQUIRE(!cp::sub_wide(-1, 2).overflow());
}
