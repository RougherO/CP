#include "cp/sys/cpu.hpp"
#include <catch2/catch_test_macros.hpp>
#include <vector>

TEST_CASE("compile-time ISA flags imply run-time support", "[cpu]") {
    if (CP_HAS_SSE42) REQUIRE(cp::sys::cpu_supports(cp::sys::cpu_feature::sse42));
    if (CP_HAS_AVX2) REQUIRE(cp::sys::cpu_supports(cp::sys::cpu_feature::avx2));
    if (CP_HAS_NEON) REQUIRE(cp::sys::cpu_supports(cp::sys::cpu_feature::neon));
    REQUIRE(cp::sys::cpu_supports(cp::sys::cpu_feature::neon) == static_cast<bool>(CP_ARCH_ARM64));
}

TEST_CASE("prefetch accepts any address", "[cpu]") {
    std::vector<int> v(64);
    cp::sys::prefetch(v.data());
    cp::sys::prefetch(nullptr);
    SUCCEED();
}
