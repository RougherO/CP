// 128-bit and wider integers, and the exact (wide) operations, against the plain built-in operators.
// Each iteration runs one operation over 1024 random operand pairs; items/s is operations per second.
#include "cp/compat/numeric.hpp"
#include "cp/int128.hpp"
#include "cp/wide.hpp"
#include <benchmark/benchmark.h>
#include <cstdint>
#include <random>
#include <vector>

namespace {

using u64          = std::uint64_t;
using u128_builtin = cp::detail::builtin_u128;
using cp::detail::u128_backport;
using u256 = cp::widen_t<cp::u128>;
using i64  = std::int64_t;

constexpr int n = 1024;

// `bits` random bits, built from 64-bit draws.
template <class T>
auto random_values(int bits, u64 seed) {
    std::mt19937_64 rng(seed);
    std::vector<T> v(n);
    for (auto& x : v) {
        T r{};
        if constexpr (cp::detail::width_v<T> > 64) {
            for (int i = 0; i < bits; i += 64) r = (r << 64) | T(rng());
        } else {
            r = static_cast<T>(rng());
        }
        x = bits < cp::detail::width_v<T> ? T(r & ((T(1) << bits) - 1)) : r;
        if (x == T{}) x = T{1};
    }
    return v;
}

template <class T, class Op>
void run(benchmark::State& state, std::vector<T> const& a, std::vector<T> const& b, Op op) {
    for (auto _ : state) {
        for (int i = 0; i < n; ++i) {
            auto r = op(a[i], b[i]);
            benchmark::DoNotOptimize(r);
        }
    }
    state.SetItemsProcessed(state.iterations() * n);
}

template <class T>
void bm_add(benchmark::State& s) {
    auto w = cp::detail::width_v<T>;
    run(s, random_values<T>(w, 1), random_values<T>(w, 2), [](T x, T y) { return x + y; });
}
template <class T>
void bm_mul(benchmark::State& s) {
    auto w = cp::detail::width_v<T>;
    run(s, random_values<T>(w, 1), random_values<T>(w, 2), [](T x, T y) { return x * y; });
}
// Divisor of `range(0)` bits: 32 (the backport's fast path, e.g. printing in base 10), half width, full width.
template <class T>
void bm_div(benchmark::State& s) {
    auto w = cp::detail::width_v<T>;
    run(s, random_values<T>(w, 1), random_values<T>(static_cast<int>(s.range(0)), 2), [](T x, T y) { return x / y; });
}

BENCHMARK_TEMPLATE(bm_add, u128_builtin);
BENCHMARK_TEMPLATE(bm_add, u128_backport);
BENCHMARK_TEMPLATE(bm_add, u256);
BENCHMARK_TEMPLATE(bm_mul, u128_builtin);
BENCHMARK_TEMPLATE(bm_mul, u128_backport);
BENCHMARK_TEMPLATE(bm_mul, u256);
BENCHMARK_TEMPLATE(bm_div, u128_builtin)->Arg(32)->Arg(64)->Arg(128);
BENCHMARK_TEMPLATE(bm_div, u128_backport)->Arg(32)->Arg(64)->Arg(128);
BENCHMARK_TEMPLATE(bm_div, u256)->Arg(32)->Arg(128)->Arg(256);

// mul_wide must cost the same as writing the widening multiply by hand.
void bm_mulhi_by_hand(benchmark::State& s) {
    run(s, random_values<u64>(64, 1), random_values<u64>(64, 2),
        [](u64 x, u64 y) { return static_cast<u64>((u128_builtin{x} * y) >> 64); });
}
void bm_mulhi_mul_wide(benchmark::State& s) {
    run(s, random_values<u64>(64, 1), random_values<u64>(64, 2), [](u64 x, u64 y) { return cp::mul_wide(x, y).high; });
}
void bm_mul_wide_u128(benchmark::State& s) {
    run(s, random_values<u128_builtin>(128, 1), random_values<u128_builtin>(128, 2),
        [](u128_builtin x, u128_builtin y) { return cp::mul_wide(x, y).high; });
}
BENCHMARK(bm_mulhi_by_hand);
BENCHMARK(bm_mulhi_mul_wide);
BENCHMARK(bm_mul_wide_u128);

// 64-bit modular multiplication, the common reason to reach for 128 bits.
void bm_mulmod(benchmark::State& s) {
    constexpr u64 m = 1'000'000'000'000'000'003ULL;
    run(s, random_values<u64>(59, 1), random_values<u64>(59, 2),
        [](u64 x, u64 y) { return static_cast<u64>(cp::mul_wide(x, y).widen() % m); });
}
BENCHMARK(bm_mulmod);

// Exact add with an overflow policy, against the plain (wrapping) add.
void bm_add_plain(benchmark::State& s) {
    run(s, random_values<i64>(63, 1), random_values<i64>(63, 2), [](i64 x, i64 y) { return x + y; });
}
void bm_add_wide(benchmark::State& s) {
    run(s, random_values<i64>(63, 1), random_values<i64>(63, 2), [](i64 x, i64 y) {
        auto r = cp::add_wide(x, y);
        return r.overflow() ? i64{0} : r.low;
    });
}
void bm_saturating_add(benchmark::State& s) {
    run(s, random_values<i64>(63, 1), random_values<i64>(63, 2), [](i64 x, i64 y) { return cp::saturating_add(x, y); });
}
BENCHMARK(bm_add_plain);
BENCHMARK(bm_add_wide);
BENCHMARK(bm_saturating_add);

}  // namespace

BENCHMARK_MAIN();
