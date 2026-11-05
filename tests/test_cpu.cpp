#include "cp/sys/cpu.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>
#if CP_ARCH_X64
#  include <cpuid.h>
#endif

TEST_CASE("compile-time ISA flags imply run-time support", "[cpu]") {
    if (CP_HAS_SSE42) REQUIRE(cp::cpu_supports(cp::cpu_feature::sse42));
    if (CP_HAS_AVX2) REQUIRE(cp::cpu_supports(cp::cpu_feature::avx2));
    if (CP_HAS_NEON) REQUIRE(cp::cpu_supports(cp::cpu_feature::neon));
}

#if CP_ARCH_X64
// The cpuid decoding only runs on MSVC; decode this machine's registers and compare with the compiler builtin.
TEST_CASE("cpuid decoding matches __builtin_cpu_supports", "[cpu]") {
    unsigned a = 0, b = 0, c = 0, d = 0;
    REQUIRE(__get_cpuid_count(1, 0, &a, &b, &c, &d));
    std::uint32_t leaf1_ecx = c;
    REQUIRE(__get_cpuid_count(7, 0, &a, &b, &c, &d));
    std::uint32_t leaf7_ebx = b;
    std::uint64_t xcr0      = 0;
    if (((leaf1_ecx >> 27) & 1U) != 0) {
        std::uint32_t lo = 0, hi = 0;
        __asm__ volatile("xgetbv" : "=a"(lo), "=d"(hi) : "c"(0));  // NOLINT(hicpp-no-assembler)
        xcr0 = (std::uint64_t{hi} << 32) | lo;
    }
    auto mask = cp::detail::decode_x86_features(leaf1_ecx, leaf7_ebx, xcr0);
    for (auto f : {cp::cpu_feature::popcnt, cp::cpu_feature::sse42, cp::cpu_feature::bmi2, cp::cpu_feature::avx2,
                   cp::cpu_feature::avx512f}) {
        CAPTURE(static_cast<int>(f));
        REQUIRE(((mask & cp::detail::feature_bit(f)) != 0) == cp::cpu_supports(f));
    }
    REQUIRE_FALSE(cp::cpu_supports(cp::cpu_feature::neon));
}
#endif

TEST_CASE("decode_x86_features requires OS support for AVX state", "[cpu]") {
    constexpr std::uint32_t osxsave = 1U << 27, avx2 = 1U << 5, avx512 = 1U << 16;
    STATIC_REQUIRE(cp::detail::decode_x86_features(osxsave, avx2, 0x6) &
                   cp::detail::feature_bit(cp::cpu_feature::avx2));
    STATIC_REQUIRE(!(cp::detail::decode_x86_features(osxsave, avx2, 0x2) &  // YMM state not enabled
                     cp::detail::feature_bit(cp::cpu_feature::avx2)));
    STATIC_REQUIRE(!(cp::detail::decode_x86_features(0, avx2, 0x6) &  // no OSXSAVE
                     cp::detail::feature_bit(cp::cpu_feature::avx2)));
    STATIC_REQUIRE(!(cp::detail::decode_x86_features(osxsave, avx512, 0x6) &  // ZMM state not enabled
                     cp::detail::feature_bit(cp::cpu_feature::avx512f)));
}

TEST_CASE("prefetch accepts any address", "[cpu]") {
    std::vector<int> v(64);
    cp::prefetch(v.data());
    cp::prefetch(nullptr);
    SUCCEED();
}
