#pragma once
// Shared by the I/O contenders: `prog read` sums the n numbers after n on stdin and prints the sum to stderr;
// `prog write N` writes N, then N pseudo-random 64-bit values of mixed lengths, one per line.
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace iobench {

// Values of every length: a 64-bit LCG, shifted right by 0–31 bits, half of them negative.
inline auto next_value(std::uint64_t& state) -> long long {
    state = state * 6364136223846793005ULL + 1442695040888963407ULL;
    return static_cast<long long>(state) >> (state & 31U);
}

inline auto reading(int argc, char** argv) -> bool { return argc > 1 && std::strcmp(argv[1], "read") == 0; }
inline auto count(int argc, char** argv) -> long long { return argc > 2 ? std::atoll(argv[2]) : 0; }

}  // namespace iobench
