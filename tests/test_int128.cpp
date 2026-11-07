#include "cp/detail/double_int.hpp"
#include "cp/num/int128.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <functional>
#include <limits>
#include <random>
#include <type_traits>
#include <utility>
#include <vector>

namespace {

using u64                = std::uint64_t;
using ubk                = cp::detail::double_uint<std::uint64_t>;
using ibk                = cp::detail::double_int<std::uint64_t>;
using cp::num::u128;  // the reference
using cp::num::i128;

constexpr auto to_bk(u128 x) { return (ubk(static_cast<u64>(x >> 64)) << 64) | ubk(static_cast<u64>(x)); }
constexpr auto to_bk(i128 x) { return ibk(to_bk(static_cast<u128>(x))); }
constexpr auto from_bk(ubk x) { return (static_cast<u128>(static_cast<u64>(x >> 64)) << 64) | static_cast<u64>(x); }
constexpr auto from_bk(ibk x) { return static_cast<i128>(from_bk(ubk(x))); }

// Edge values plus random values of every bit length, so carries, borrows and both division paths all run.
auto samples() {
    std::vector<u128> v{
        0, 1, 2, 3, ~u64{0}, u128{~u64{0}} + 1, u128{1} << 63, u128{1} << 127, ~u128{0}, ~u128{0} - 1, ~u128{0} >> 1};
    std::mt19937_64 rng(42);
    for (int bits = 1; bits <= 128; ++bits) {
        for (int k = 0; k < 2; ++k) {
            u128 x = (static_cast<u128>(rng()) << 64) | rng();
            v.push_back(bits == 128 ? x : x & ((u128{1} << bits) - 1));
        }
    }
    return v;
}

template <class T>
using plus_t = decltype(std::declval<T>() + std::declval<T>());
template <class A, class B, class = void>
inline constexpr bool can_add_v = false;
template <class A, class B>
inline constexpr bool can_add_v<A, B, std::void_t<decltype(std::declval<A>() + std::declval<B>())>> = true;

}  // namespace

TEST_CASE("u128 backport matches unsigned __int128", "[int128]") {
    auto values = samples();
    for (u128 a : values) {
        ubk x = to_bk(a);
        CAPTURE(static_cast<u64>(a >> 64), static_cast<u64>(a));
        REQUIRE(from_bk(~x) == static_cast<u128>(~a));
        REQUIRE(from_bk(-x) == static_cast<u128>(-a));
        for (int n : {0, 1, 31, 63, 64, 65, 100, 127}) {
            CAPTURE(n);
            REQUIRE(from_bk(x << n) == static_cast<u128>(a << n));
            REQUIRE(from_bk(x >> n) == static_cast<u128>(a >> n));
        }
        REQUIRE(static_cast<double>(x) == static_cast<double>(a));
        REQUIRE(static_cast<float>(x) == static_cast<float>(a));
        REQUIRE(static_cast<long double>(x) == static_cast<long double>(a));
        auto d = static_cast<double>(a);
        if (d < 0x1p128) REQUIRE(from_bk(ubk(d)) == static_cast<u128>(d));

        for (u128 b : values) {
            ubk y = to_bk(b);
            CAPTURE(static_cast<u64>(b >> 64), static_cast<u64>(b));
            REQUIRE(from_bk(x + y) == static_cast<u128>(a + b));
            REQUIRE(from_bk(x - y) == static_cast<u128>(a - b));
            REQUIRE(from_bk(x * y) == static_cast<u128>(a * b));
            REQUIRE(from_bk(x & y) == (a & b));
            REQUIRE(from_bk(x | y) == (a | b));
            REQUIRE(from_bk(x ^ y) == (a ^ b));
            REQUIRE((x == y) == (a == b));
            REQUIRE((x < y) == (a < b));
            REQUIRE((x >= y) == (a >= b));
            if (b != 0) {
                REQUIRE(from_bk(x / y) == a / b);
                REQUIRE(from_bk(x % y) == a % b);
            }
        }
    }
}

TEST_CASE("i128 backport matches __int128", "[int128]") {
    auto values = samples();
    for (u128 ua : values) {
        auto a = static_cast<i128>(ua);
        ibk x  = to_bk(a);
        CAPTURE(static_cast<u64>(ua >> 64), static_cast<u64>(ua));
        for (int n : {0, 1, 63, 64, 65, 127}) {
            CAPTURE(n);
            REQUIRE(from_bk(x >> n) == (a >> n));
        }
        REQUIRE(static_cast<double>(x) == static_cast<double>(a));
        REQUIRE(static_cast<long double>(x) == static_cast<long double>(a));
        auto d = static_cast<double>(a);
        if (-0x1p127 <= d && d < 0x1p127) REQUIRE(from_bk(ibk(d)) == static_cast<i128>(d));

        for (u128 ub : values) {
            auto b = static_cast<i128>(ub);
            ibk y  = to_bk(b);
            CAPTURE(static_cast<u64>(ub >> 64), static_cast<u64>(ub));
            REQUIRE((x == y) == (a == b));
            REQUIRE((x < y) == (a < b));
            REQUIRE((x > y) == (a > b));
            if (b != 0 && !(a == std::numeric_limits<i128>::min() && b == -1)) {
                REQUIRE(from_bk(x / y) == a / b);
                REQUIRE(from_bk(x % y) == a % b);
            }
        }
    }
}

TEST_CASE("128-bit backports work like built-in integers", "[int128]") {
    // Mixed with built-in integers; negatives wrap or sign-extend as for __int128.
    STATIC_REQUIRE(ubk(5) + 1 == 6);
    STATIC_REQUIRE(1 + ubk(5) == 6);
    STATIC_REQUIRE(ubk(-1) == ~ubk(0));
    STATIC_REQUIRE(ibk(-7) / 2 == -3);
    STATIC_REQUIRE(ibk(-7) % 2 == -1);
    STATIC_REQUIRE((ibk(-8) >> 1) == -4);
    STATIC_REQUIRE(static_cast<int>(ibk(-5)) == -5);
    STATIC_REQUIRE(static_cast<u64>(ubk(~u64{0}) + 1) == 0);
    STATIC_REQUIRE(!ubk(0));
    STATIC_REQUIRE((ubk(1) << 127) > ubk(~u64{0}));

    STATIC_REQUIRE([] {
        ubk x  = 10;
        x     += 5;
        x     *= 3;
        ++x;
        ubk old = x--;
        return old == 46 && x == 45;
    }());

    // Converting back, or mixing signed with unsigned, needs a cast (code that compiles here also compiles with
    // __int128).
    STATIC_REQUIRE(!std::is_convertible_v<ubk, u64>);
    STATIC_REQUIRE(!std::is_convertible_v<ibk, ubk>);
    STATIC_REQUIRE(!can_add_v<ibk, ubk>);
    STATIC_REQUIRE(std::is_same_v<plus_t<ubk>, ubk>);
    STATIC_REQUIRE(std::is_trivially_copyable_v<ubk>);

    STATIC_REQUIRE(from_bk(std::numeric_limits<ubk>::max()) == std::numeric_limits<u128>::max());
    STATIC_REQUIRE(from_bk(std::numeric_limits<ibk>::min()) == std::numeric_limits<i128>::min());
    STATIC_REQUIRE(from_bk(std::numeric_limits<ibk>::max()) == std::numeric_limits<i128>::max());
    REQUIRE(std::hash<ubk>{}(ubk(1) << 64) != std::hash<ubk>{}(ubk(1)));
}
