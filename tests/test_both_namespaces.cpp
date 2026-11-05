// Solutions often open both namespaces. Every public name must still resolve unqualified, on every language level:
// when cp aliases std the two names are one entity, and when cp backports, std has no such name yet.
#include "cp/all.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <vector>

using namespace std;  // NOLINT(google-build-using-namespace): the point of this test
using namespace cp;   // NOLINT(google-build-using-namespace)

namespace {
enum class colour : std::uint8_t { red = 2 };
template <class T>
using size_expr = decltype(std::declval<T&>().size());
}  // namespace

TEST_CASE("every public name resolves with std and cp both open", "[all]") {
    // <bit>
    std::uint32_t x = 40;
    REQUIRE(bit_cast<std::uint32_t>(1.0f) == 0x3f80'0000U);
    REQUIRE(bit_ceil(x) == 64U);
    REQUIRE(bit_floor(x) == 32U);
    REQUIRE(bit_width(x) == 6);
    REQUIRE(byteswap(std::uint16_t{0x0102}) == 0x0201);
    REQUIRE(countl_zero(x) == 26);
    REQUIRE(countl_one(~x) == 26);
    REQUIRE(countr_zero(x) == 3);
    REQUIRE(countr_one(x >> 3) == 1);
    REQUIRE(has_single_bit(32U));
    REQUIRE(popcount(x) == 2);
    REQUIRE(rotl(1U, 1) == 2U);
    REQUIRE(rotr(2U, 1) == 1U);
    REQUIRE((endian::native == endian::little || endian::native == endian::big));

    // <utility>, <numeric>, <iterator>
    REQUIRE(cmp_less(-1, 0U));
    REQUIRE(cmp_equal(1, 1U));
    REQUIRE(cmp_not_equal(-1, 0U));
    REQUIRE(cmp_greater(1U, -1));
    REQUIRE(cmp_less_equal(0, 0U));
    REQUIRE(cmp_greater_equal(0U, 0));
    REQUIRE(in_range<std::uint8_t>(255));
    REQUIRE(to_underlying(colour::red) == 2);
    REQUIRE(midpoint(2, 6) == 4);
    REQUIRE(saturating_add(std::int8_t{120}, std::int8_t{10}) == 127);
    REQUIRE(saturating_sub(0U, 1U) == 0U);
    REQUIRE(saturating_mul(std::int8_t{-100}, std::int8_t{2}) == -128);
    REQUIRE(saturating_div(-128, -1) == 128);
    REQUIRE(saturating_cast<std::uint8_t>(300) == 255);
    std::vector<int> v{3, 1, 2};
    REQUIRE(ssize(v) == 3);
    if (v.empty()) unreachable();

    // <type_traits> backports and the detection idiom
    STATIC_REQUIRE(is_same_v<remove_cvref_t<int const&>, int>);
    STATIC_REQUIRE(is_same_v<type_identity_t<int>, int>);
    STATIC_REQUIRE(is_detected_v<size_expr, std::vector<int>>);
    STATIC_REQUIRE(!is_detected_v<size_expr, int>);
    STATIC_REQUIRE(is_same_v<detected_t<size_expr, int>, nonesuch>);
    STATIC_REQUIRE(is_detected<size_expr, std::vector<int>>::value);
    REQUIRE_FALSE(is_constant_evaluated());

    // span, source_location, ranges
    span<int> s(v);
    REQUIRE(s.size() == 3);
    REQUIRE(span<int, dynamic_extent>(v).size() == 3);
    REQUIRE(as_bytes(s).size() == 3 * sizeof(int));
    REQUIRE(as_writable_bytes(s).size() == 3 * sizeof(int));
    REQUIRE(source_location::current().line() > 0);
    int sum = 0;
    for (int i : views::iota(0, 4)) sum += i;
    for (int i : views::reverse(v)) sum += i;
    REQUIRE(sum == 12);

    // inplace_vector and its free erase functions
    inplace_vector<int, 4> iv{1, 2, 2, 3};
    REQUIRE(erase(iv, 2) == 2);
    REQUIRE(erase_if(iv, [](int e) { return e == 3; }) == 1);
    REQUIRE(iv.size() == 1);

    // 128-bit and wider integers
    using namespace cp::literals;  // NOLINT(google-build-using-namespace)
    u128 big = 18446744073709551616_u128;
    i128 neg = -5_i128;
    REQUIRE(big == u128{1} << 64);
    REQUIRE(neg < 0);
    widen_t<u128> huge = big;
    REQUIRE(huge * huge == widen_t<u128>(1) << 128);
    wide<std::uint64_t> w = mul_wide(~std::uint64_t{0}, std::uint64_t{2});
    REQUIRE(w.high == 1);
    REQUIRE(add_wide(~0U, 1U).overflow());
    REQUIRE(sub_wide(0U, 1U).widen() == ~std::uint64_t{0});

    // CPU
    REQUIRE((cpu_supports(cpu_feature::neon) || !cpu_supports(cpu_feature::neon)));
    prefetch(v.data());
}
