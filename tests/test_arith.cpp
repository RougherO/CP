#include "cp/compat/iterator.hpp"
#include "cp/compat/numeric.hpp"
#include "cp/compat/utility.hpp"
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cfloat>
#include <climits>
#include <cstdint>
#include <limits>
#include <random>
#include <type_traits>
#include <vector>

TEST_CASE("mixed-sign comparisons give the mathematically right answer", "[utility]") {
    STATIC_REQUIRE(cp::cmp_less(-1, 0u));
    STATIC_REQUIRE(!cp::cmp_equal(-1, UINT_MAX));
    STATIC_REQUIRE(cp::cmp_greater(UINT64_MAX, INT64_MAX));
    STATIC_REQUIRE(cp::cmp_less_equal(INT64_MIN, 0u));
    STATIC_REQUIRE(!cp::in_range<std::uint8_t>(256));
    STATIC_REQUIRE(cp::in_range<std::int8_t>(-128));
    STATIC_REQUIRE(!cp::in_range<std::int8_t>(-129));
    STATIC_REQUIRE(!cp::in_range<unsigned>(-1));
}

TEST_CASE("midpoint does not overflow and rounds towards the first argument", "[numeric]") {
    STATIC_REQUIRE(cp::midpoint(INT_MIN, INT_MAX) == -1);
    STATIC_REQUIRE(cp::midpoint(INT_MAX, INT_MIN) == 0);
    STATIC_REQUIRE(cp::midpoint(1, 4) == 2);
    STATIC_REQUIRE(cp::midpoint(4, 1) == 3);
    STATIC_REQUIRE(cp::midpoint(std::uint8_t{0}, std::uint8_t{255}) == 127);
    STATIC_REQUIRE(cp::midpoint(UINT64_MAX, std::uint64_t{0}) == UINT64_MAX / 2 + 1);
    REQUIRE(cp::midpoint(DBL_MAX, DBL_MAX) == DBL_MAX);
    REQUIRE(cp::midpoint(-DBL_MAX, DBL_MAX) == 0.0);
    int arr[5]{};
    REQUIRE(cp::midpoint(arr, arr + 5) == arr + 2);
    REQUIRE(cp::midpoint(arr + 5, arr) == arr + 3);
}

TEST_CASE("saturating arithmetic clamps at the edges", "[numeric]") {
    STATIC_REQUIRE(cp::saturating_add(INT_MAX, 1) == INT_MAX);
    STATIC_REQUIRE(cp::saturating_add(INT_MIN, -1) == INT_MIN);
    STATIC_REQUIRE(cp::saturating_sub(0u, 1u) == 0u);
    STATIC_REQUIRE(cp::saturating_sub(INT_MIN, 1) == INT_MIN);
    STATIC_REQUIRE(cp::saturating_sub(0, INT_MIN) == INT_MAX);
    STATIC_REQUIRE(cp::saturating_mul(INT_MIN, -1) == INT_MAX);
    STATIC_REQUIRE(cp::saturating_mul(INT_MIN, 2) == INT_MIN);
    STATIC_REQUIRE(cp::saturating_div(INT_MIN, -1) == INT_MAX);
    STATIC_REQUIRE(cp::saturating_div(7, 2) == 3);
    STATIC_REQUIRE(cp::saturating_cast<std::uint8_t>(300) == 255);
    STATIC_REQUIRE(cp::saturating_cast<std::int8_t>(-1000) == -128);
    STATIC_REQUIRE(cp::saturating_cast<unsigned>(-5) == 0u);
}

TEMPLATE_TEST_CASE("saturating add/sub/mul clamp the exact result, all 8-bit pairs", "[numeric]", std::int8_t,
                   std::uint8_t) {
    using T    = TestType;
    using L    = std::numeric_limits<T>;
    auto clamp = [](int x) { return static_cast<T>(x < L::min() ? L::min() : x > L::max() ? L::max() : x); };
    for (int a = int{L::min()}; a <= int{L::max()}; ++a) {
        for (int b = int{L::min()}; b <= int{L::max()}; ++b) {
            CAPTURE(a, b);
            REQUIRE(cp::saturating_add(T(a), T(b)) == clamp(a + b));
            REQUIRE(cp::saturating_sub(T(a), T(b)) == clamp(a - b));
            REQUIRE(cp::saturating_mul(T(a), T(b)) == clamp(a * b));
        }
    }
}

TEST_CASE("ssize and to_underlying", "[iterator][utility]") {
    std::vector<int> v(3);
    int arr[5]{};
    STATIC_REQUIRE(std::is_same_v<decltype(cp::ssize(v)), std::ptrdiff_t>);
    REQUIRE(cp::ssize(v) == 3);
    REQUIRE(cp::ssize(arr) == 5);
    enum class small : short { a = -3 };  // NOLINT(performance-enum-size): short is the point
    STATIC_REQUIRE(std::is_same_v<decltype(cp::to_underlying(small::a)), short>);
    STATIC_REQUIRE(cp::to_underlying(small::a) == -3);
}
