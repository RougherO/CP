#pragma once
/// @file
/// @brief Run-time CPU feature detection (cp::cpu_supports) and cache prefetch (cp::prefetch).
/// @par Caveats
/// - cp::cpu_supports answers for the machine the program runs on; CP_HAS_AVX2 and friends (config.hpp) answer for
///   the flags it was compiled with. Pair a `CP_TARGET("avx2")` function with a `cpu_supports` check.
/// - x86 features include the operating system's support for their registers (AVX state saving).
/// - On ARM64, NEON is always reported (the architecture requires it) and every x86 feature is false.
/// - MSVC decodes `cpuid` itself (best-effort, not tested on MSVC); GCC and Clang use `__builtin_cpu_supports`.
#include "cp/config.hpp"
#include <cstdint>
#if CP_MSVC && CP_ARCH_X64
#  include <intrin.h>
#endif

namespace cp {

/// @brief A CPU feature that cp::cpu_supports can query.
enum class cpu_feature : std::uint8_t {
    popcnt,   ///< x86 POPCNT
    sse42,    ///< x86 SSE4.2
    bmi2,     ///< x86 BMI2 (`pdep`, `pext`)
    avx2,     ///< x86 AVX2
    avx512f,  ///< x86 AVX-512 Foundation
    neon,     ///< ARM NEON
};

namespace detail {

/// @brief Bit `f` of the mask: set if feature `f` is usable.
constexpr auto feature_bit(cpu_feature f) noexcept -> std::uint32_t {
    return std::uint32_t{1} << static_cast<unsigned>(f);
}

/// @brief Usable x86 features from `cpuid` leaf 1 ECX, leaf 7 (sub-leaf 0) EBX and `xgetbv(0)` (XCR0), as a
/// feature_bit mask. AVX features also need the OS to save their registers (OSXSAVE plus the XCR0 state bits).
constexpr auto decode_x86_features(std::uint32_t leaf1_ecx, std::uint32_t leaf7_ebx, std::uint64_t xcr0) noexcept
    -> std::uint32_t {
    auto has           = [](std::uint64_t reg, int bit) { return ((reg >> bit) & 1U) != 0; };
    bool os_avx        = has(leaf1_ecx, 27) && (xcr0 & 0x6U) == 0x6U;  // OSXSAVE; XMM and YMM state
    bool os_avx512     = os_avx && (xcr0 & 0xe0U) == 0xe0U;            // opmask, ZMM_Hi256, Hi16_ZMM state
    std::uint32_t mask = 0;
    if (has(leaf1_ecx, 23)) mask |= feature_bit(cpu_feature::popcnt);
    if (has(leaf1_ecx, 20)) mask |= feature_bit(cpu_feature::sse42);
    if (has(leaf7_ebx, 8)) mask |= feature_bit(cpu_feature::bmi2);
    if (os_avx && has(leaf7_ebx, 5)) mask |= feature_bit(cpu_feature::avx2);
    if (os_avx512 && has(leaf7_ebx, 16)) mask |= feature_bit(cpu_feature::avx512f);
    return mask;
}

#if CP_MSVC && CP_ARCH_X64
/// @brief The running CPU's features, read once.
inline auto msvc_cpu_features() noexcept -> std::uint32_t {
    static std::uint32_t const mask = [] {
        int r1[4]{}, r7[4]{};  // NOLINT(modernize-avoid-c-arrays): __cpuidex takes int[4]
        __cpuidex(r1, 1, 0);
        __cpuidex(r7, 7, 0);
        auto ecx           = static_cast<std::uint32_t>(r1[2]);
        std::uint64_t xcr0 = ((ecx >> 27) & 1U) != 0 ? _xgetbv(0) : 0;
        return decode_x86_features(ecx, static_cast<std::uint32_t>(r7[1]), xcr0);
    }();
    return mask;
}
#endif

}  // namespace detail

/// @brief True if the running CPU (and OS) can execute instructions of feature `f`.
/// @code
/// CP_TARGET("avx2") void sum_avx2(cp::span<int const> v);
/// if (cp::cpu_supports(cp::cpu_feature::avx2)) sum_avx2(v); else sum_scalar(v);
/// @endcode
inline auto cpu_supports(cpu_feature f) noexcept -> bool {
#if CP_ARCH_ARM64
    return f == cpu_feature::neon;
#elif CP_MSVC
    return (detail::msvc_cpu_features() & detail::feature_bit(f)) != 0;
#else
    switch (f) {
        case cpu_feature::popcnt:
            return __builtin_cpu_supports("popcnt");
        case cpu_feature::sse42:
            return __builtin_cpu_supports("sse4.2");
        case cpu_feature::bmi2:
            return __builtin_cpu_supports("bmi2");
        case cpu_feature::avx2:
            return __builtin_cpu_supports("avx2");
        case cpu_feature::avx512f:
            return __builtin_cpu_supports("avx512f");
        case cpu_feature::neon:
            return false;
    }
    return false;
#endif
}

/// @brief Hint that the cache line at `p` will be read soon. Never faults, even for an invalid address.
inline void prefetch(void const* p) noexcept {
#if CP_MSVC && CP_ARCH_X64
    _mm_prefetch(static_cast<char const*>(p), _MM_HINT_T0);
#elif CP_MSVC
    static_cast<void>(p);
#else
    __builtin_prefetch(p, 0, 3);
#endif
}

}  // namespace cp
