#include "cp/detail/double_int.hpp"
#include "cp/io/format.hpp"
#include "cp/io/parse.hpp"
#include "cp/num/int128.hpp"
#include "cp/sys/input_source.hpp"
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
#include <random>
#include <string>

namespace {

using cp::detail::parse_status;

// The input as the reader presents it: followed by the zero padding.
auto padded(std::string const& s) -> std::string { return s + std::string(cp::detail::input_padding, '\0'); }

template <class T>
struct parsed {
    T value{};
    std::size_t length = 0;
    parse_status status{};
};

template <class T>
auto parse(std::string const& s, int base = 10) -> parsed<T> {
    auto buf = padded(s);
    parsed<T> r;
    auto res =
        base == 10 ? cp::detail::parse_int(buf.data(), r.value) : cp::detail::parse_int(buf.data(), r.value, base);
    r.length = static_cast<std::size_t>(res.end - buf.data());
    r.status = res.status;
    return r;
}

template <class T>
auto format(T v, int base = 10) -> std::string {
    std::string s(static_cast<std::size_t>(cp::detail::max_int_chars_v<T>), '\0');
    char* p   = s.data();
    if (base == 10) {
        p = cp::detail::format_int(p, v);
    } else {
        if (v < 0) *p++ = '-';
        p = cp::detail::format_unsigned(p, cp::detail::unsigned_abs(v), base);
    }
    char* end = p;
    s.resize(static_cast<std::size_t>(end - s.data()));
    return s;
}

// A decimal string plus one.
auto increment(std::string s) -> std::string {
    auto i = s.size();
    while (i > 0 && s[i - 1] == '9') s[--i] = '0';
    if (i == 0) return '1' + s;
    ++s[i - 1];
    return s;
}

template <class T>
auto random_value(std::mt19937_64& rng) -> T {
    using U = cp::detail::unsigned_of_t<T>;
    U u{};
    for (int i = 0; i < cp::detail::width_v<T>; i += 64) {
        if constexpr (cp::detail::width_v<T> > 64) u = u << 64;
        u = u | static_cast<U>(rng());
    }
    auto bits = static_cast<U>(u >> static_cast<int>(rng() % static_cast<unsigned>(cp::detail::width_v<T>)));
    return static_cast<T>(bits);  // small and large magnitudes, both signs
}

using i128_backport = cp::detail::double_int<std::uint64_t>;  // the portable 128-bit type, checked against __int128
using u128_backport = cp::detail::double_uint<std::uint64_t>;

}  // namespace

TEST_CASE("decimal integers of every length match std::from_chars", "[io][parse]") {
    std::mt19937_64 rng(1);
    for (int len = 1; len <= 20; ++len) {
        for (int trial = 0; trial < 200; ++trial) {
            std::string s(static_cast<std::size_t>(len), '0');
            for (auto& ch : s) ch = static_cast<char>('0' + rng() % 10);
            std::uint64_t expected = 0;
            auto ref               = std::from_chars(s.data(), s.data() + s.size(), expected);
            auto got               = parse<std::uint64_t>(s + " 7");
            CAPTURE(s);
            REQUIRE(got.length == s.size());
            if (ref.ec == std::errc{}) {
                REQUIRE(got.status == parse_status::ok);
                REQUIRE(got.value == expected);
            } else {
                REQUIRE(got.status == parse_status::overflow);
            }
        }
    }
}

TEMPLATE_TEST_CASE("integer limits parse; one past them overflows", "[io][parse]", std::int8_t, std::uint8_t,
                   std::int16_t, std::uint16_t, std::int32_t, std::uint32_t, std::int64_t, std::uint64_t, cp::num::i128,
                   cp::num::u128, i128_backport, u128_backport) {
    using L       = std::numeric_limits<TestType>;
    auto max_text = format(L::max());
    auto r        = parse<TestType>(max_text);
    REQUIRE(r.status == parse_status::ok);
    REQUIRE(r.value == L::max());
    REQUIRE(parse<TestType>("+" + max_text).value == L::max());
    REQUIRE(parse<TestType>(increment(max_text)).status == parse_status::overflow);
    REQUIRE(parse<TestType>(increment(max_text)).length == increment(max_text).size());  // whole token consumed
    if constexpr (L::is_signed) {
        REQUIRE(format(L::min()) == "-" + increment(max_text));
        auto m = parse<TestType>("-" + increment(max_text));
        REQUIRE(m.status == parse_status::ok);
        REQUIRE(m.value == L::min());
        REQUIRE(parse<TestType>("-" + increment(increment(max_text))).status == parse_status::overflow);
        REQUIRE(parse<TestType>("-0").value == TestType{});
    } else {
        REQUIRE(parse<TestType>("-1").status == parse_status::invalid);
    }
    // Leading zeros past the overflow-free length must not count as overflow.
    auto z = parse<TestType>(std::string(45, '0') + "42");
    REQUIRE(z.status == parse_status::ok);
    REQUIRE(z.value == TestType{42});
}

TEST_CASE("128-bit limits as text", "[io][parse]") {
    REQUIRE(format(std::numeric_limits<cp::num::u128>::max()) == "340282366920938463463374607431768211455");
    REQUIRE(format(std::numeric_limits<cp::num::i128>::min()) == "-170141183460469231731687303715884105728");
    REQUIRE(format(std::numeric_limits<u128_backport>::max()) == "340282366920938463463374607431768211455");
    REQUIRE(format(cp::num::u128{std::numeric_limits<std::uint64_t>::max()} + 1) == "18446744073709551616");
}

TEST_CASE("not a number", "[io][parse]") {
    for (char const* s : {"", "+", "-", "-x", "abc", "+-1", "--1", " 1"}) {
        CAPTURE(s);
        auto r = parse<int>(s);
        REQUIRE(r.status == parse_status::invalid);
        REQUIRE(r.length == 0);
    }
    auto r = parse<int>("12abc");
    REQUIRE((r.status == parse_status::ok && r.value == 12 && r.length == 2));
    REQUIRE(parse<int>("7\r\n").length == 1);
}

TEMPLATE_TEST_CASE("random integers round-trip in every base", "[io][parse]", std::int8_t, std::uint16_t, std::int32_t,
                   std::int64_t, std::uint64_t, cp::num::i128, cp::num::u128, i128_backport, u128_backport) {
    std::mt19937_64 rng(2);
    for (int i = 0; i < 20000; ++i) {
        auto v    = random_value<TestType>(rng);
        auto base = i % 3 == 0 ? 10 : 2 + static_cast<int>(rng() % 35);
        auto text = format(v, base);
        CAPTURE(text, base);
        if constexpr (cp::detail::width_v<TestType> <= 64) {
            std::string ref(80, '\0');
            ref.resize(
                static_cast<std::size_t>(std::to_chars(ref.data(), ref.data() + ref.size(), v, base).ptr - ref.data()));
            REQUIRE(text == ref);
        }
        auto r = parse<TestType>(text, base);
        REQUIRE(r.status == parse_status::ok);
        REQUIRE(r.value == v);
        REQUIRE(r.length == text.size());
    }
    REQUIRE(parse<int>("ff", 16).value == 255);
    REQUIRE(parse<int>("FF", 16).value == 255);
    REQUIRE(format(255, 16) == "ff");
}

TEST_CASE("floats round-trip and parse like std::from_chars", "[io][parse]") {
    std::mt19937_64 rng(3);
    std::string buf(400, '\0');
    for (int i = 0; i < 20000; ++i) {
        auto bits = rng();
        double v  = 0;
        std::memcpy(&v, &bits, sizeof v);
        if (!std::isfinite(v)) continue;
        char* end = cp::detail::format_float(buf.data(), buf.data() + buf.size(), v, std::chars_format::general, -1);
        REQUIRE(end != nullptr);
        std::string text(buf.data(), end);
        double back = 0;
        auto in     = padded(text);
        REQUIRE(cp::detail::parse_float(in.data(), back).status == parse_status::ok);
        CAPTURE(text);
        REQUIRE(back == v);
    }
    double x   = 0;
    auto check = [&](std::string const& s, parse_status st) {
        auto in = padded(s);
        CAPTURE(s);
        REQUIRE(cp::detail::parse_float(in.data(), x).status == st);
    };
    check("+1.5", parse_status::ok);
    REQUIRE(x == 1.5);
    check("-inf", parse_status::ok);
    REQUIRE(std::isinf(x));
    check("nan", parse_status::ok);
    check("1e400", parse_status::overflow);
    check("abc", parse_status::invalid);
    check("+-1", parse_status::invalid);
}

TEST_CASE("fixed float output matches printf", "[io][format]") {
    std::string buf(400, '\0'), ref(400, '\0');
    for (double v : {0.0, -0.5, 1.0 / 3, 123456789.987654321, 1e-12, -2.5e15, 0.125}) {
        char* end = cp::detail::format_float(buf.data(), buf.data() + buf.size(), v, std::chars_format::fixed, 9);
        int n     = std::snprintf(ref.data(), ref.size(), "%.9f", v);
        REQUIRE(std::string(buf.data(), end) == ref.substr(0, static_cast<std::size_t>(n)));
    }
    // The bound holds for the longest outputs.
    auto bound = cp::detail::max_float_chars<double>(std::chars_format::fixed, -1);
    std::string big(static_cast<std::size_t>(bound), '\0');
    for (double v : {-std::numeric_limits<double>::max(), -std::numeric_limits<double>::denorm_min()}) {
        REQUIRE(cp::detail::format_float(big.data(), big.data() + big.size(), v, std::chars_format::fixed, -1));
    }
}

TEST_CASE("count_digits at every power of ten", "[io][format]") {
    REQUIRE(cp::detail::count_digits(0) == 1);
    for (std::size_t i = 1; i < cp::detail::pow10_u64.size(); ++i) {
        CAPTURE(i);
        REQUIRE(cp::detail::count_digits(cp::detail::pow10_u64[i] - 1) == static_cast<int>(i));
        REQUIRE(cp::detail::count_digits(cp::detail::pow10_u64[i]) == static_cast<int>(i) + 1);
    }
    REQUIRE(cp::detail::count_digits(std::numeric_limits<std::uint64_t>::max()) == 20);
}
