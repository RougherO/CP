// Number parsing and formatting over an in-memory buffer (no OS calls): cp against std::from_chars / std::to_chars,
// a plain digit loop, and printf. items/s is numbers per second.
// Two inputs: "small" is uniform in [0, 10^9) like most contest input; "full" is uniform over int64_t (19 digits).
#include "cp/io/format.hpp"
#include "cp/io/parse.hpp"
#include "cp/sys/input_source.hpp"
#include <benchmark/benchmark.h>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

namespace {

using i64 = std::int64_t;

constexpr int n = 1 << 20;

auto values(bool full) -> std::vector<i64> {
    std::mt19937_64 rng(42);
    std::vector<i64> v(n);
    for (auto& x : v) x = full ? static_cast<i64>(rng()) : static_cast<i64>(rng() % 1'000'000'000);
    return v;
}

// The values as space-separated text, followed by the reader's zero padding.
auto text(bool full) -> std::string {
    std::string s;
    for (auto x : values(full)) s += std::to_string(x) + ' ';
    return s + std::string(cp::detail::input_padding, '\0');
}

template <class Parse>
void run_parse(benchmark::State& state, Parse parse) {
    auto const s = text(state.range(0) != 0);
    for (auto _ : state) {
        char const* p = s.data();
        i64 sum       = 0;
        for (int i = 0; i < n; ++i) {
            i64 x  = 0;
            p      = parse(p, x) + 1;  // skip the space
            sum   += x;
        }
        benchmark::DoNotOptimize(sum);
    }
    state.SetItemsProcessed(state.iterations() * n);
}

void parse_cp(benchmark::State& s) {
    run_parse(s, [](char const* p, i64& x) { return cp::detail::parse_int(p, x).end; });
}
void parse_from_chars(benchmark::State& s) {
    // from_chars needs the token's end; give it a bound 32 bytes on, as a caller without a sentinel would.
    run_parse(s, [](char const* p, i64& x) { return std::from_chars(p, p + 32, x).ptr; });
}
void parse_loop(benchmark::State& s) {
    run_parse(s, [](char const* p, i64& x) {
        bool neg         = *p == '-';
        p               += neg ? 1 : 0;
        std::uint64_t r  = 0;
        for (; static_cast<unsigned>(*p - '0') < 10; ++p) r = r * 10 + static_cast<unsigned>(*p - '0');
        x = neg ? -static_cast<i64>(r) : static_cast<i64>(r);
        return p;
    });
}
BENCHMARK(parse_cp)->Arg(0)->Arg(1);
BENCHMARK(parse_from_chars)->Arg(0)->Arg(1);
BENCHMARK(parse_loop)->Arg(0)->Arg(1);

template <class Format>
void run_format(benchmark::State& state, Format format) {
    auto const v = values(state.range(0) != 0);
    std::string out(static_cast<std::size_t>(n) * 24, '\0');
    for (auto _ : state) {
        char* p = out.data();
        for (auto x : v) {
            p    = format(p, x);
            *p++ = ' ';
        }
        benchmark::DoNotOptimize(p);
        benchmark::ClobberMemory();
    }
    state.SetItemsProcessed(state.iterations() * n);
}

void format_cp(benchmark::State& s) {
    run_format(s, [](char* p, i64 x) { return cp::detail::format_int(p, x); });
}
void format_to_chars(benchmark::State& s) {
    run_format(s, [](char* p, i64 x) { return std::to_chars(p, p + 24, x).ptr; });
}
void format_snprintf(benchmark::State& s) {
    run_format(s, [](char* p, i64 x) { return p + std::snprintf(p, 24, "%lld", static_cast<long long>(x)); });
}
BENCHMARK(format_cp)->Arg(0)->Arg(1);
BENCHMARK(format_to_chars)->Arg(0)->Arg(1);
BENCHMARK(format_snprintf)->Arg(0)->Arg(1);

}  // namespace

BENCHMARK_MAIN();
