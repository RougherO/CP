#include "cp/num/modint.hpp"
#include "cp/num/consts.hpp"
#include "fd_helpers.hpp"
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstdint>
#include <limits>
#include <random>

namespace {

using u64 = std::uint64_t;
using cp::num::i128;
using cp::num::u128;

// The reference: plain 128-bit arithmetic.
auto ref_mod(i128 x, u64 m) -> u64 {
    i128 const r = x % static_cast<i128>(m);
    return static_cast<u64>(r < 0 ? r + static_cast<i128>(m) : r);
}

// Near the top of each storage width: the sum of two residues wraps the type before reaching m.
using m32top = cp::num::modint<4'294'967'291>;  // largest prime below 2^32
using m61    = cp::num::modint<(u64{1} << 61) - 1>;
using m64top = cp::num::modint<18'446'744'073'709'551'557ULL>;  // largest prime below 2^64

}  // namespace

TEST_CASE("modint construction maps every integer into [0, M)", "[modint]") {
    using cp::num::modint998;
    STATIC_REQUIRE(modint998(-1).value() == 998'244'352);
    STATIC_REQUIRE(modint998(998'244'353).value() == 0);
    REQUIRE(modint998(std::numeric_limits<std::int64_t>::min()).value() ==
            ref_mod(std::numeric_limits<std::int64_t>::min(), 998'244'353));
    REQUIRE(m64top(std::numeric_limits<i128>::min()).value() ==
            ref_mod(std::numeric_limits<i128>::min(), m64top::mod()));
    REQUIRE(m64top(~u64{0}).value() == ~u64{0} - m64top::mod());
    STATIC_REQUIRE(cp::num::modint<1>(12345).value() == 0);
}

TEMPLATE_TEST_CASE("modint + - * agree with 128-bit arithmetic", "[modint]", cp::num::modint998, m32top, m61, m64top) {
    std::mt19937_64 rng(42);
    u64 const m = TestType::mod();
    for (int i = 0; i < 20'000; ++i) {
        u64 const a = i < 4 ? m - 1 - static_cast<u64>(i) : rng() % m;  // the top residues first
        u64 const b = i < 4 ? m - 1 : rng() % m;
        TestType x(a), y(b);
        REQUIRE((x + y).value() == static_cast<u64>((u128{a} + b) % m));
        REQUIRE((x - y).value() == ref_mod(i128{a} - i128{b}, m));
        REQUIRE((x * y).value() == static_cast<u64>(u128{a} * b % m));
    }
}

TEST_CASE("modint inv, / and pow work for any modulus coprime to the value", "[modint]") {
    using m1000 = cp::num::modint<1000>;  // not prime: Fermat's little theorem would be wrong here
    STATIC_REQUIRE((m1000(7).inv() * 7) == 1);
    STATIC_REQUIRE(m1000(3) / 7 * 7 == 3);
    STATIC_REQUIRE(m1000(3).pow(-2) * 9 == 1);
    STATIC_REQUIRE(m1000(2).pow(0) == 1);
    STATIC_REQUIRE((m64top(5).inv() * 5) == 1);
    STATIC_REQUIRE(m61(3).pow(m61::mod() - 1) == 1);
    STATIC_REQUIRE(-m1000(0) == 0 && -m1000(1) == 999);
    auto x = m1000(999);
    REQUIRE(++x == 0);
    REQUIRE(--x == 999);
}

TEST_CASE("dyn_modint's Barrett reduction agrees with % across the modulus range", "[modint]") {
    using dm = cp::num::dyn_modint<7>;
    std::mt19937_64 rng(7);
    for (u64 m : {u64{1}, u64{2}, u64{3}, u64{998'244'353}, u64{2'147'483'647}, u64{4'294'967'291}, u64{4'294'967'295},
                  u64{1} << 31}) {
        dm::set_mod(m);
        for (int i = 0; i < 20'000; ++i) {
            u64 const a = i == 0 ? m - 1 : rng() % m, b = i == 0 ? m - 1 : rng() % m;
            REQUIRE((dm(a) * dm(b)).value() == a * b % m);
            REQUIRE((dm(a) + dm(b)).value() == (a + b) % m);
        }
    }
    dm::set_mod(1'000'000'007);
    REQUIRE(dm(-5).value() == 1'000'000'002);
    REQUIRE(cp::num::dyn_modint<8>::mod() == 998'244'353);  // each Id has its own modulus
}

#if CP_CHECKS
TEST_CASE("modint inv reports a value that has no inverse", "[modint]") {
    auto r = test::run_child("", [] { [[maybe_unused]] auto y = cp::num::modint<1000>(10).inv(); });
    REQUIRE(r.aborted);
    REQUIRE(r.err.find("not coprime") != std::string::npos);
}
#endif

TEST_CASE("consts: std::numbers values, inf survives addition", "[consts]") {
    namespace k = cp::num::consts;
    REQUIRE(k::pi == std::acos(-1.0));
    REQUIRE(k::e == std::exp(1.0));
    REQUIRE(k::sqrt2 == std::sqrt(2.0));
    REQUIRE(k::ln2 == std::log(2.0));
    STATIC_REQUIRE(k::tau == 2 * k::pi);
    STATIC_REQUIRE(k::pi_v<float> == static_cast<float>(k::pi));
    STATIC_REQUIRE(k::inf + k::inf > k::inf);
    STATIC_REQUIRE(k::inf_v<int> == std::numeric_limits<int>::max() / 2);
    STATIC_REQUIRE(k::inf_v<double> == std::numeric_limits<double>::infinity());
    STATIC_REQUIRE(k::mersenne61 == 2'305'843'009'213'693'951);
    for (std::int64_t q : {2, 7, 17}) {  // mod998 - 1 = 2^23 * 7 * 17: a generator has no smaller order
        REQUIRE(cp::num::modint998(k::ntt_root).pow((k::mod998 - 1) / q) != 1);
    }
}
