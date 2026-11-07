#pragma once
/// @file
/// @brief Run-time CPU feature detection (cp::sys::cpu_supports) and cache prefetch (cp::sys::prefetch).
/// @par Caveats
/// - cp::sys::cpu_supports answers for the machine the program runs on; CP_HAS_AVX2 and friends (config.hpp) answer for
///   the flags it was compiled with. Pair a `CP_TARGET("avx2")` function with a `cpu_supports` check.
/// - x86 features include the operating system's support for their registers (AVX state saving).
/// - On ARM64, NEON is always reported (the architecture requires it) and every x86 feature is false.
#include "cp/config.hpp"
#include <cstdint>

namespace cp::sys {

/// @brief A CPU feature that cp::sys::cpu_supports can query.
enum class cpu_feature : std::uint8_t {
    popcnt,   ///< x86 POPCNT
    sse42,    ///< x86 SSE4.2
    bmi2,     ///< x86 BMI2 (`pdep`, `pext`)
    avx2,     ///< x86 AVX2
    avx512f,  ///< x86 AVX-512 Foundation
    neon,     ///< ARM NEON
};

/// @brief True if the running CPU (and OS) can execute instructions of feature `f`.
/// @code
/// CP_TARGET("avx2") void sum_avx2(cp::ds::span<int const> v);
/// if (cp::sys::cpu_supports(cp::sys::cpu_feature::avx2)) sum_avx2(v); else sum_scalar(v);
/// @endcode
inline auto cpu_supports(cpu_feature f) noexcept -> bool {
#if CP_ARCH_ARM64
    return f == cpu_feature::neon;
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
inline void prefetch(void const* p) noexcept { __builtin_prefetch(p, 0, 3); }

}  // namespace cp::sys
