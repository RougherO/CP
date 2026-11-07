// Modular multiplication: cp's modint types against plain `%`. Each iteration does 1024 multiplications;
// items/s is multiplications per second. `independent`: a[i]·b[i] (throughput); `chain`: x = x·a[i] (latency, as
// in a running product or pow).
#include "cp/num/int128.hpp"
#include "cp/num/modint.hpp"
#include <benchmark/benchmark.h>
#include <cstdint>
#include <random>
#include <vector>

namespace {

using u64 = std::uint64_t;
using cp::num::u128;

constexpr int n    = 1024;
constexpr u64 p998 = 998'244'353;
constexpr u64 p61  = (u64{1} << 61) - 1;
using dyn          = cp::num::dyn_modint<>;

auto residues(u64 m, u64 seed) {
    std::mt19937_64 rng(seed);
    std::vector<u64> v(n);
    for (auto& x : v) x = rng() % m;
    return v;
}

// a[i]·b[i], independent.
template <class T>
void run_independent(benchmark::State& state, u64 m) {
    std::vector<T> a, b;
    for (u64 x : residues(m, 1)) a.push_back(T(x));
    for (u64 x : residues(m, 2)) b.push_back(T(x));
    for (auto _ : state) {
        for (int i = 0; i < n; ++i) {
            auto r = a[i] * b[i];
            benchmark::DoNotOptimize(r);
        }
    }
    state.SetItemsProcessed(state.iterations() * n);
}
// x = x·a[i], each multiplication waiting for the previous one.
template <class T>
void run_chain(benchmark::State& state, u64 m) {
    std::vector<T> a;
    for (u64 x : residues(m, 1)) a.push_back(T(x));
    for (auto _ : state) {
        T x(1);
        for (int i = 0; i < n; ++i) x = x * a[i];
        benchmark::DoNotOptimize(x);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

// Baselines: what a hand-written `a * b % MOD` costs.
struct plain998 {  // u64 product, % by a constant
    u64 v;
    explicit plain998(u64 x) : v(x) { }
    friend auto operator*(plain998 a, plain998 b) { return plain998(a.v * b.v % p998); }
};
struct plain_dyn {  // u64 product, % by a run-time value
    u64 v;
    static inline u64 m = p998;
    explicit plain_dyn(u64 x) : v(x) { }
    friend auto operator*(plain_dyn a, plain_dyn b) { return plain_dyn(a.v * b.v % m); }
};
struct plain61 {  // 128-bit product, % by a constant
    u64 v;
    explicit plain61(u64 x) : v(x) { }
    friend auto operator*(plain61 a, plain61 b) { return plain61(static_cast<u64>(u128{a.v} * b.v % p61)); }
};

void bm_plain998(benchmark::State& s) { run_independent<plain998>(s, p998); }
void bm_modint998(benchmark::State& s) { run_independent<cp::num::modint998>(s, p998); }
void bm_plain_dyn(benchmark::State& s) { run_independent<plain_dyn>(s, p998); }
void bm_dyn_modint(benchmark::State& s) { run_independent<dyn>(s, p998); }
void bm_plain61(benchmark::State& s) { run_independent<plain61>(s, p61); }
void bm_modint61(benchmark::State& s) { run_independent<cp::num::modint<p61>>(s, p61); }

void bm_chain_plain998(benchmark::State& s) { run_chain<plain998>(s, p998); }
void bm_chain_modint998(benchmark::State& s) { run_chain<cp::num::modint998>(s, p998); }
void bm_chain_plain_dyn(benchmark::State& s) { run_chain<plain_dyn>(s, p998); }
void bm_chain_dyn_modint(benchmark::State& s) { run_chain<dyn>(s, p998); }
void bm_chain_modint61(benchmark::State& s) { run_chain<cp::num::modint<p61>>(s, p61); }

}  // namespace

BENCHMARK(bm_plain998);
BENCHMARK(bm_modint998);
BENCHMARK(bm_plain_dyn);
BENCHMARK(bm_dyn_modint);
BENCHMARK(bm_plain61);
BENCHMARK(bm_modint61);
BENCHMARK(bm_chain_plain998);
BENCHMARK(bm_chain_modint998);
BENCHMARK(bm_chain_plain_dyn);
BENCHMARK(bm_chain_dyn_modint);
BENCHMARK(bm_chain_modint61);

BENCHMARK_MAIN();
