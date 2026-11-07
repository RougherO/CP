// Solutions often open std and cp's modules together. Every public name must still resolve unqualified, on every
// language level: where cp's name equals a std name it is that std entity, and every other name has no std namesake.
#include "cp/aliases/aliases.hpp"
#include "cp/all.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>
#include <vector>

// NOLINTBEGIN(google-build-using-namespace): the point of this test
using namespace std;
using namespace cp;
using namespace cp::aliases;
using namespace cp::bits;
using namespace cp::debug;
using namespace cp::ds;
using namespace cp::io;
using namespace cp::num;
using namespace cp::num::consts;
#if defined(__cpp_lib_math_constants)
using namespace std::numbers;  // the same entities as cp::num::consts' std names
#endif
using namespace cp::sys;
using namespace cp::traits;
using namespace cp::util;
// NOLINTEND(google-build-using-namespace)

namespace {
enum class colour : std::uint8_t { red = 2 };
template <class T>
using size_expr = decltype(std::declval<T&>().size());
}  // namespace

TEST_CASE("every public name resolves with std and every cp module open", "[all]") {
    // bits
    std::uint32_t x = 40;
    REQUIRE(cast<std::uint32_t>(1.0f) == 0x3f80'0000U);
    REQUIRE(ceil2(x) == 64U);
    REQUIRE(floor2(x) == 32U);
    REQUIRE(width(x) == 6);
    REQUIRE(bswap(std::uint16_t{0x0102}) == 0x0201);
    REQUIRE(clz(x) == 26);
    REQUIRE(clo(~x) == 26);
    REQUIRE(ctz(x) == 3);
    REQUIRE(cto(x >> 3) == 1);
    REQUIRE(is_pow2(32U));
    REQUIRE(popcnt(x) == 2);
    REQUIRE(rotl(1U, 1) == 2U);
    REQUIRE(rotr(2U, 1) == 1U);
    REQUIRE((endian::native == endian::little || endian::native == endian::big));

    // num: comparison, saturation, midpoint
    REQUIRE(cmp(-1, 0U) < 0);
    REQUIRE(cmp(1, 1U) == strong_ordering::equal);
    REQUIRE(cmp(0.5, 0.25) == partial_ordering::greater);
    weak_ordering w = strong_ordering::less;
    REQUIRE(w < 0);
    REQUIRE(fits<std::uint8_t>(255));
    REQUIRE(midpoint(2, 6) == 4);
    REQUIRE(add_sat(std::int8_t{120}, std::int8_t{10}) == 127);
    REQUIRE(sub_sat(0U, 1U) == 0U);
    REQUIRE(mul_sat(std::int8_t{-100}, std::int8_t{2}) == -128);
    REQUIRE(div_sat(-128, -1) == 128);
    REQUIRE(cast_sat<std::uint8_t>(300) == 255);

    // num: 128-bit and wider integers
    u128 big = 18446744073709551616_u128;
    i128 neg = -5_i128;
    REQUIRE(big == u128{1} << 64);
    REQUIRE(neg < 0);
    widen_t<u128> huge = big;
    REQUIRE(huge * huge == widen_t<u128>(1) << 128);
    wide<std::uint64_t> wd = mul_wide(~std::uint64_t{0}, std::uint64_t{2});
    REQUIRE(wd.high == 1);
    REQUIRE(add_wide(~0U, 1U).overflow());
    REQUIRE(sub_wide(0U, 1U).widen() == ~std::uint64_t{0});

    // traits
    STATIC_REQUIRE(is_same_v<remove_cvref_t<int const&>, int>);
    STATIC_REQUIRE(is_same_v<type_identity_t<int>, int>);
    STATIC_REQUIRE(is_detected_v<size_expr, std::vector<int>>);
    STATIC_REQUIRE(!is_detected_v<size_expr, int>);
    STATIC_REQUIRE(is_same_v<detected_t<size_expr, int>, nonesuch>);
    STATIC_REQUIRE(is_specialization_of_v<std::vector<int>, std::vector>);
    STATIC_REQUIRE(!is_specialization_of_v<int, std::vector>);
    STATIC_REQUIRE(is_detected<size_expr, std::vector<int>>::value);
    REQUIRE_FALSE(is_constant_evaluated());
    REQUIRE(to_underlying(colour::red) == 2);

    // ds, views, debug
    std::vector<int> v{3, 1, 2};
    REQUIRE(ssize(v) == 3);
    span<int> s(v);
    REQUIRE(s.size() == 3);
    REQUIRE(span<int, dynamic_extent>(v).size() == 3);
#if defined(__cpp_lib_span)  // std::span only
    REQUIRE(as_bytes(s).size() == 3 * sizeof(int));
    REQUIRE(as_writable_bytes(s).size() == 3 * sizeof(int));
#endif
    result<int, char> ok = 4, bad = failure('e');
    REQUIRE((ok && *ok == 4 && !bad && bad.error() == 'e' && bad.value_or(1) == 1));
    static_vector<int, 4> sv{1, 2, 2, 3};
    REQUIRE(erase(sv, 2) == 2);
    REQUIRE(erase_if(sv, [](int elem) { return elem == 3; }) == 1);
    REQUIRE(sv.size() == 1);
    int sum = 0;
    for (int i : views::iota(0, 4)) sum += i;
    for (int i : views::reverse(v)) sum += i;
    REQUIRE(sum == 12);
    REQUIRE(source_location::current().line() > 0);
    if (v.empty()) unreachable();
    check(!v.empty(), "v");
    CP_CHECK(v.size() == 3);
    [[maybe_unused]] auto trace = [&] { dump(precision(3), v, sum); };

    // sys
    REQUIRE((cpu_supports(cpu_feature::neon) || !cpu_supports(cpu_feature::neon)));
    prefetch(v.data());

    // io: compiled, not run (the test's stdin may be a terminal)
    [[maybe_unused]] auto io = [] {
        int n = 0;
        std::string line;
        std::vector<long long> a;
        read(n, a);
        auto k        = *read<int>();
        auto [h, wid] = *read<int, int>();
        getline(line);
        readln(n);
        readln();
        read(base(16), n, defaults(), a);
        while (read(n)) {
        }
        if (auto r = read(n); !r && r.error() == read_error::invalid) n = 0;
        write(n, a);
        writeln(k, h, wid);
        out.flush();
        err.write(line);
        auto hex = *read<int>(base(16) | float_format(chars_format::general) | skip_ws(false));
        writeln((range_sep(",") | field_sep(","))(a), hex,
                width(2) | fill('0') | show_plus() | show_bool_words() | show_quotes() | show_field_names() | wrap(80) |
                    range_bracket("[", "]") | field_bracket("(", ")") | arg_sep(" ") |
                    float_format(chars_format::fixed) | precision(3),
                1);
        [[maybe_unused]] formatting<> identity = defaults();
        reader r(0);
        writer o(1);
        static_cast<void>(r), static_cast<void>(o);
    };

    // aliases
    i64 a1   = 5_i64;
    u64 b1   = 0xff_u64;
    usize c1 = 3_uz;
    isize d1 = -4_iz;
    STATIC_REQUIRE(is_unsigned_v<u16> && is_signed_v<i8> && sizeof(u32) == 4);
    REQUIRE(a1 + static_cast<i64>(b1) + static_cast<i64>(c1) + d1 == 259);

    // the modules by name, after `using namespace cp;`
    REQUIRE(bits::popcnt(7U) == 3);
    REQUIRE(num::cmp(-1, 0U) < 0);
    STATIC_REQUIRE(std::numeric_limits<num::u128>::digits == 128);

    // num: modint and consts, unqualified
    REQUIRE((modint998(2) * inf).value() == static_cast<u64>(2 * (inf % mod998) % mod998));
    REQUIRE(dyn_modint<3>(7).pow(2) == 49);
    STATIC_REQUIRE(pi > 3 && pi_v<float> > 3 && tau > pi && mod1e9_7 > mod998);
}
