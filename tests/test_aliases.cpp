#include "cp/aliases/aliases.hpp"
#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <type_traits>

using namespace cp::aliases;  // NOLINT(google-build-using-namespace)

TEST_CASE("literals have their alias type", "[aliases]") {
    STATIC_REQUIRE(std::is_same_v<decltype(1_i32), i32>);
    STATIC_REQUIRE(std::is_same_v<decltype(1_u32), u32>);
    STATIC_REQUIRE(std::is_same_v<decltype(1_u128), u128>);
    STATIC_REQUIRE(std::is_same_v<decltype(1_i128), i128>);
    STATIC_REQUIRE(std::is_same_v<i128, cp::num::i128>);
    STATIC_REQUIRE(4294967295_u32 == std::numeric_limits<u32>::max());
}

TEST_CASE("128-bit literals", "[aliases]") {
    STATIC_REQUIRE(340282366920938463463374607431768211455_u128 == ~u128{0});
    STATIC_REQUIRE(170141183460469231731687303715884105727_i128 == std::numeric_limits<i128>::max());
    STATIC_REQUIRE(0xffff'ffff'ffff'ffff'ffff'ffff'ffff'ffff_u128 == ~u128{0});
    STATIC_REQUIRE(0b1000_u128 == 8);
    STATIC_REQUIRE(017_u128 == 15);
    STATIC_REQUIRE(-5_i128 == -5);
    STATIC_REQUIRE(18446744073709551616_u128 == u128{1} << 64);
}
