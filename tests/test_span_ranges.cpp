#include "cp/views/views.hpp"
#include "cp/ds/span.hpp"
#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <numeric>
#include <type_traits>
#include <utility>
#include <vector>

TEST_CASE("span constructor rules match std::span", "[span]") {
    // If these differ, code breaks when cp::ds::span becomes std::span.
    STATIC_REQUIRE(std::is_convertible_v<std::vector<int>&, cp::ds::span<int>>);
    STATIC_REQUIRE(!std::is_constructible_v<cp::ds::span<int>, std::vector<int>&&>);  // would dangle
    STATIC_REQUIRE(std::is_convertible_v<std::vector<int>&&, cp::ds::span<int const>>);
    STATIC_REQUIRE(!std::is_constructible_v<cp::ds::span<int>, std::vector<int> const&>);
    STATIC_REQUIRE(std::is_convertible_v<cp::ds::span<int>, cp::ds::span<int const>>);
    STATIC_REQUIRE(!std::is_convertible_v<cp::ds::span<int const>, cp::ds::span<int>>);
    STATIC_REQUIRE(std::is_trivially_copyable_v<cp::ds::span<int>>);
#if defined(__cpp_lib_span)                                                           // static extents: std::span only
    STATIC_REQUIRE(!std::is_convertible_v<std::vector<int>&, cp::ds::span<int, 3>>);  // explicit
    STATIC_REQUIRE(std::is_constructible_v<cp::ds::span<int, 3>, std::vector<int>&>);
    STATIC_REQUIRE(!std::is_constructible_v<cp::ds::span<int, 4>, std::array<int, 3>&>);
#endif
}

TEST_CASE("span deduction and slicing", "[span]") {
    std::vector<int> v{1, 2, 3, 4, 5};
    std::array<int, 3> a{7, 8, 9};
    int c[4]{1, 1, 2, 3};

    cp::ds::span sv(v);
    cp::ds::span sa(a);
    cp::ds::span sc(c);
    cp::ds::span sca(std::as_const(a));
    STATIC_REQUIRE(decltype(sv)::extent == cp::ds::dynamic_extent);
    STATIC_REQUIRE(std::is_same_v<decltype(sca)::element_type, int const>);
    REQUIRE(sc.size() == 4);

    cp::ds::span it(v.begin(), v.end());
    cp::ds::span it2(v.begin() + 1, 2);
    REQUIRE(it.size() == 5);
    REQUIRE(it2.front() == 2);
    REQUIRE(it2.back() == 3);
    REQUIRE(sv.first(2)[1] == 2);
    REQUIRE(sv.last(2)[0] == 4);
    REQUIRE(sv.subspan(1, 3).size() == 3);
    REQUIRE(sv.subspan(2).front() == 3);
    REQUIRE(sv.subspan(5).empty());
    REQUIRE(cp::ds::span(v.begin(), v.begin()).empty());

    cp::ds::span<int const> ro = sa;
    REQUIRE(std::accumulate(ro.begin(), ro.end(), 0) == 24);
    REQUIRE(*sv.rbegin() == 5);
}

TEST_CASE("iota is random access and follows std's bound rules", "[ranges]") {
    auto r = cp::views::iota(0, 5);
    REQUIRE(std::vector<int>(r.begin(), r.end()) == std::vector<int>{0, 1, 2, 3, 4});
    REQUIRE(std::distance(r.begin(), r.end()) == 5);
    REQUIRE(*std::lower_bound(r.begin(), r.end(), 3) == 3);

    // Binary search over iota is O(log n).
    auto big = cp::views::iota(10LL, 20LL);
    REQUIRE(*std::partition_point(big.begin(), big.end(), [](long long x) { return x * x < 200; }) == 15);

    // Same signedness required; different types give an end sentinel.
    STATIC_REQUIRE(!std::is_invocable_v<decltype(cp::views::iota) const&, int, std::size_t>);
    auto mixed = cp::views::iota(0, 5LL);
    STATIC_REQUIRE(!std::is_same_v<decltype(mixed.begin()), decltype(mixed.end())>);
    int sum = 0;
    for (int x : mixed) sum += x;
    REQUIRE(sum == 10);

    int count = 0;
    for (auto x : cp::views::iota(5u)) {
        if (x == 8u) break;
        ++count;
    }
    REQUIRE(count == 3);

    auto ru = cp::views::iota(2u, 5u);
    REQUIRE(ru.end() - ru.begin() == 3);
    REQUIRE(ru.begin() - ru.end() == -3);
}

TEST_CASE("reverse keeps lvalues by reference and owns rvalues", "[ranges]") {
    std::vector<int> v{1, 2, 3, 4, 5};
    std::vector<int> out;
    for (int x : v | cp::views::reverse) out.push_back(x);
    REQUIRE(out == std::vector<int>{5, 4, 3, 2, 1});

    out.clear();
    for (int x : cp::views::reverse(std::vector<int>{1, 2, 3})) out.push_back(x);  // no dangling
    REQUIRE(out == std::vector<int>{3, 2, 1});

    out.clear();
    for (int x : cp::views::iota(0, 3) | cp::views::reverse) out.push_back(x);
    REQUIRE(out == std::vector<int>{2, 1, 0});
}
