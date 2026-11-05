#include "cp/compat/bit.hpp"
#include "cp/detail/double_int.hpp"
#include <bitset>
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <limits>
#include <random>
#include <vector>

namespace {

// References built on std::bitset: no bit tricks to get wrong.
template <class T>
auto ref_clz(T x) {
    std::bitset<64> bits(x);
    int n = 0;
    for (int i = std::numeric_limits<T>::digits - 1; i >= 0 && !bits[i]; --i) ++n;
    return n;
}
template <class T>
auto ref_ctz(T x) {
    std::bitset<64> bits(x);
    int n = 0;
    for (int i = 0; i < std::numeric_limits<T>::digits && !bits[i]; ++i) ++n;
    return n;
}
template <class T>
auto ref_pop(T x) {
    return static_cast<int>(std::bitset<64>(x).count());
}

// Every value for 8/16-bit types; edges plus random values for wider ones.
template <class T>
auto inputs() {
    std::vector<T> xs;
    if constexpr (sizeof(T) <= 2) {
        for (unsigned v = 0; v <= std::numeric_limits<T>::max(); ++v) xs.push_back(T(v));
    } else {
        std::mt19937_64 rng(12345);
        xs = {T(0), T(1), T(~T(0))};
        for (unsigned i = 0; i < unsigned(std::numeric_limits<T>::digits); ++i) xs.push_back(T(std::uint64_t{1} << i));
        for (int i = 0; i < 100000; ++i) xs.push_back(T(rng() >> (rng() % 64)));
    }
    return xs;
}

}  // namespace

TEMPLATE_TEST_CASE("bit operations match a loop reference", "[bit]", std::uint8_t, std::uint16_t, std::uint32_t,
                   std::uint64_t) {
    using T             = TestType;
    constexpr int width = std::numeric_limits<T>::digits;
    for (T x : inputs<T>()) {
        CAPTURE(x);
        REQUIRE(cp::countl_zero(x) == ref_clz(x));
        REQUIRE(cp::countr_zero(x) == ref_ctz(x));
        REQUIRE(cp::countl_one(x) == ref_clz(T(~x)));
        REQUIRE(cp::countr_one(x) == ref_ctz(T(~x)));
        REQUIRE(cp::popcount(x) == ref_pop(x));
        REQUIRE(cp::has_single_bit(x) == (ref_pop(x) == 1));
        REQUIRE(cp::bit_width(x) == width - ref_clz(x));
        // The portable versions only run on MSVC, so check them against the intrinsics here.
        REQUIRE(cp::detail::countl_zero_portable(x) == ref_clz(x));
        REQUIRE(cp::detail::countr_zero_portable(x) == ref_ctz(x));
        REQUIRE(cp::detail::popcount_portable(x) == ref_pop(x));
        REQUIRE(cp::detail::byteswap_portable(x) == cp::byteswap(x));
    }
}

TEST_CASE("bit edge cases at compile time", "[bit]") {
    STATIC_REQUIRE(cp::countl_zero(std::uint8_t{0}) == 8);
    STATIC_REQUIRE(cp::countr_zero(std::uint16_t{0}) == 16);
    STATIC_REQUIRE(cp::countl_zero(std::uint8_t{1}) == 7);
    STATIC_REQUIRE(cp::bit_floor(0u) == 0u);
    STATIC_REQUIRE(cp::bit_ceil(0u) == 1u);
    STATIC_REQUIRE(cp::bit_ceil(1u) == 1u);
    STATIC_REQUIRE(cp::bit_ceil(5u) == 8u);
    STATIC_REQUIRE(cp::bit_floor(5u) == 4u);
    STATIC_REQUIRE(cp::rotl(std::uint8_t{0b1000'0001}, 1) == 0b0000'0011);
    STATIC_REQUIRE(cp::rotl(std::uint8_t{0b1000'0001}, -1) == cp::rotr(std::uint8_t{0b1000'0001}, 1));
    STATIC_REQUIRE(cp::rotl(std::uint8_t{0b1000'0001}, 9) == cp::rotl(std::uint8_t{0b1000'0001}, 1));
    STATIC_REQUIRE(cp::byteswap(std::uint64_t{0x0102'0304'0506'0708}) == 0x0807'0605'0403'0201ull);
    STATIC_REQUIRE(cp::detail::byteswap_portable(std::uint32_t{0x0102'0304}) == 0x0403'0201u);
    REQUIRE(cp::bit_cast<std::uint32_t>(1.0f) == 0x3f80'0000u);
}

#if CP_HAS_INT128
TEST_CASE("bit functions count all 128 bits of unsigned __int128", "[bit]") {
    using u128 = cp::detail::builtin_u128;
    STATIC_REQUIRE(cp::countl_zero(u128{1} << 100) == 27);
    STATIC_REQUIRE(cp::countr_zero(u128{1} << 100) == 100);
    STATIC_REQUIRE(cp::popcount(~u128{0}) == 128);
    STATIC_REQUIRE(cp::bit_width(u128{1} << 100) == 101);
}
#endif
