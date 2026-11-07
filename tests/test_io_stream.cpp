#include "cp/io/reader.hpp"
#include "cp/io/writer.hpp"
#include "cp/num/int128.hpp"
#include "fd_helpers.hpp"
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstdio>
#include <map>
#include <random>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

using namespace test;  // NOLINT(google-build-using-namespace): test helpers

namespace user {
// A hook that reads the fields in reverse, so a test can tell it from the automatic struct path.
struct point {
    int x, y;
};
inline void cp_read(cp::io::reader& r, point& p) { r.read(p.y, p.x); }
inline void cp_write(cp::io::writer& w, point const& p) {
    w.put('(');
    w.write(p.x);
    w.put(',');
    w.write(p.y);
    w.put(')');
}
}  // namespace user

namespace {

struct record {
    int id;
    std::string name;
    std::array<int, 2> pos;
    std::pair<char, double> tag;
};

// Writes `text` to a pipe (closed afterwards) and returns a reader over it with `capacity`.
struct piped_input {
    test_pipe pipe;
    cp::io::reader in;
    piped_input(std::string const& text, std::size_t capacity) : in(pipe.read_end(), capacity) {
        pipe.write(text);
        pipe.close_write();
    }
};

struct file_input {
    temp_file file;
    cp::io::reader in;
    explicit file_input(std::string const& text) : file(text), in(file.fd()) { }
};

// What a writer produced, after flushing it.
template <class F>
auto written(F&& f, std::size_t capacity = std::size_t{1} << 16) -> std::string {
    temp_file file("");
    {
        cp::io::writer out(file.fd(), capacity);
        f(out);
    }
    return contents(file);
}

using token = std::variant<std::int64_t, double, std::string, char>;

auto random_tokens(std::mt19937_64& rng, int count) -> std::pair<std::string, std::vector<token>> {
    static constexpr std::array<char const*, 6> spaces{" ", "\n", "\r\n", "\t", "  \n ", " \r\n\r\n"};
    std::string text;
    std::vector<token> tokens;
    for (int i = 0; i < count; ++i) {
        switch (rng() % 4) {
            case 0: {
                auto v  = static_cast<std::int64_t>(rng()) >> (rng() % 64);
                text   += std::to_string(v);
                tokens.emplace_back(v);
                break;
            }
            case 1: {
                double v =
                    static_cast<double>(static_cast<std::int64_t>(rng())) / static_cast<double>(1 + rng() % 1000);
                std::array<char, 40> buf{};
                text += std::string(buf.data(),
                                    static_cast<std::size_t>(std::snprintf(buf.data(), buf.size(), "%.17g", v)));
                tokens.emplace_back(v);
                break;
            }
            case 2: {
                std::string s(1 + rng() % 100, 'a');
                for (auto& c : s) c = static_cast<char>('a' + rng() % 26);
                text += s;
                tokens.emplace_back(s);
                break;
            }
            default: {
                auto c  = static_cast<char>('A' + rng() % 26);
                text   += c;
                tokens.emplace_back(c);
            }
        }
        text += spaces[rng() % spaces.size()];
    }
    return {text, tokens};
}

void read_and_check(cp::io::reader& in, std::vector<token> const& tokens) {
    for (auto const& t : tokens) {
        std::visit(
            [&](auto const& expected) {
                std::decay_t<decltype(expected)> got{};
                REQUIRE(in.read(got));
                REQUIRE(got == expected);
            },
            t);
    }
    std::string extra;
    REQUIRE_FALSE(in.read(extra));
    REQUIRE(in.eof());
}

}  // namespace

// Small buffers split tokens at every possible point between refills.
TEST_CASE("random token streams read the same from pipes of any buffer size and from files", "[io][stream]") {
    std::mt19937_64 rng(7);
    auto [text, tokens] = random_tokens(rng, 600);
    REQUIRE(text.size() < 60000);  // fits in the pipe before anything reads it
    for (std::size_t capacity : {32, 33, 37, 47, 64, 100, 4096}) {
        CAPTURE(capacity);
        piped_input p(text, capacity);
        read_and_check(p.in, tokens);
    }
    file_input f(text);
    read_and_check(f.in, tokens);
}

TEST_CASE("end of input ends a read loop exactly once", "[io][stream]") {
    for (char const* text : {"1 2 3", "1 2 3\n", "  1\n2\r\n3 \n\n"}) {
        CAPTURE(text);
        piped_input p(text, 64);
        long long sum = 0, x = 0;
        int reads = 0;
        while (p.in.read(x)) sum += x, ++reads;
        REQUIRE((sum == 6 && reads == 3));
        REQUIRE(p.in.eof());
    }
    for (char const* text : {"", " \n\t "}) {
        file_input f(text);
        int x = 0;
        REQUIRE_FALSE(f.in.read(x));
        REQUIRE(f.in.eof());
    }
}

TEST_CASE("readln drops the rest of the line", "[io][stream]") {
    piped_input p("5 junk\nhello world\n7\n8", 64);
    int n = 0, k = 0, j = 0;
    std::string line;
    REQUIRE(p.in.readln(n));
    REQUIRE(p.in.getline(line));
    REQUIRE((n == 5 && line == "hello world"));
    REQUIRE(p.in.readln(k));  // the token ends the line: only its '\n' goes
    REQUIRE(p.in.readln(j));  // no trailing '\n' at end of input
    REQUIRE((k == 7 && j == 8));
    REQUIRE_FALSE(p.in.readln(j));
    piped_input q("a b\nc\n", 64);
    std::string w;
    REQUIRE((q.in.readln() && q.in.read(w) && w == "c"));  // readln() skips to the next line
}

TEST_CASE("getline follows std::getline and drops \\r", "[io][stream]") {
    file_input f("5\nhello world\r\n\nlast");
    int n = 0;
    std::string line;
    f.in.read(n);
    f.in.getline(line);
    REQUIRE(line.empty());  // the rest of the line after 5, as with std::getline
    REQUIRE(f.in.getline(line));
    REQUIRE(line == "hello world");
    REQUIRE(f.in.getline(line));
    REQUIRE(line.empty());
    REQUIRE(f.in.getline(line));
    REQUIRE(line == "last");
    REQUIRE(f.in.eof());
    REQUIRE_FALSE(f.in.getline(line));
}

TEST_CASE("compound types read element by element", "[io][stream]") {
    file_input f("3 1 2 3  1 0 1  4 5  x 2.5  7 bob 8 9 q 0.5  10 20  1 2 3 4  200 -5");
    std::vector<int> v(3);
    std::vector<bool> b(3);
    std::pair<int, int> pr;
    std::tuple<char, double> tp;
    record r{};
    user::point pt{};
    std::vector<std::vector<int>> grid(2, std::vector<int>(2));
    std::uint8_t u8 = 0;
    std::int8_t i8  = 0;
    int n           = 0;
    REQUIRE(f.in.read(n, v, b, pr, tp, r, pt, grid, u8, i8));
    REQUIRE(v == std::vector<int>{1, 2, 3});
    REQUIRE(b == std::vector<bool>{true, false, true});
    REQUIRE(pr == std::pair{4, 5});
    REQUIRE(tp == std::tuple{'x', 2.5});
    REQUIRE((r.id == 7 && r.name == "bob" && r.pos == std::array{8, 9} && r.tag == std::pair{'q', 0.5}));
    REQUIRE((pt.x == 20 && pt.y == 10));  // the hook, not the struct path
    REQUIRE(grid == std::vector<std::vector<int>>{{1, 2}, {3, 4}});
    REQUIRE((u8 == 200 && i8 == -5));  // numbers, not characters
}

TEST_CASE("chars skip whitespace unless told not to", "[io][stream]") {
    file_input f(" a b");
    char c = 0;
    f.in.read(c);
    REQUIRE(c == 'a');
    f.in.read(cp::io::skip_ws(false), c);
    REQUIRE(c == ' ');
    REQUIRE(f.in.read(c));  // the next call skips again
    REQUIRE(c == 'b');
}

TEST_CASE("reading through formatters", "[io][format]") {
    using namespace cp::io;
    file_input f("ff -101");
    int a = 0, b = 0;
    REQUIRE(f.in.read(base(16), a, base(2), b));
    REQUIRE((a == 255 && b == -5));
    piped_input p("ff 11 ff", 64);
    int x = 0, y = 0, z = 0;
    REQUIRE(p.in.read(base(16), x, base(2), y));
    REQUIRE((x == 255 && y == 3));
    REQUIRE_FALSE(p.in.read(z));  // the next call reads base 10: "ff" is invalid
    piped_input q("x 7", 64);
    char ch = 0, sp = 0;
    int d = 0;
    REQUIRE(q.in.read(ch, skip_ws(false)(sp), d));
    REQUIRE((sp == ' ' && d == 7));
}

// The judge sends a line and waits for the answer: the reader must not ask for more than the token needs.
TEST_CASE("a complete token is returned without waiting for more input", "[io][stream]") {
    test_pipe pipe;
    cp::io::reader in(pipe.read_end(), 64);
    pipe.write("42\n");  // the write end stays open, as with a waiting judge
    int x = 0;
    REQUIRE(in.read(x));
    REQUIRE(x == 42);
}

TEST_CASE("the tied writer is flushed before the reader waits for input", "[io][stream]") {
    test_pipe input, output;
    cp::io::writer out(output.write_end());
    cp::io::reader in(input.read_end(), 64, &out);
    out.write("query");  // buffered
    input.write("7\n");
    int x = 0;
    in.read(x);  // the first read waits on the pipe: flushes "query" first
    REQUIRE(x == 7);
    output.close_write();  // a missing flush then reads as empty instead of blocking
    REQUIRE(read_all(output.read_end()) == "query");
}

TEST_CASE("bad input returns its error, stops later reads until clear(), and stays unread", "[io][stream]") {
    file_input f("1\n2 abc 4\n");
    int a = 0, b = 0, c = 0;
    auto r = f.in.read(a, b, c);
    REQUIRE((!r && r.error() == cp::io::read_error::invalid));
    REQUIRE(f.in.token() == "abc");
    REQUIRE_FALSE(f.in.eof());
    REQUIRE(f.in.read(a).error() == cp::io::read_error::invalid);  // sticky
    f.in.clear();
    std::string s;
    REQUIRE((f.in.read(s, c) && s == "abc" && c == 4));
#if CP_CHECKS
    REQUIRE(f.in.line() == 2);
#endif
    file_input g("300");
    std::uint8_t x = 0;
    REQUIRE(g.in.read(x).error() == cp::io::read_error::out_of_range);
    REQUIRE(g.in.token() == "300");
}

TEST_CASE("writer formats values, ranges and structs with the separators", "[io][stream]") {
    REQUIRE(written([](cp::io::writer& w) { w.write(1, "ab", 'c', 2.5, std::string("s"), true); }) ==
            "1 ab c 2.500000000 s 1");
    REQUIRE(written([](cp::io::writer& w) { w.writeln(std::vector{1, 2, 3}).writeln(); }) == "1 2 3\n\n");
    REQUIRE(written([](cp::io::writer& w) { w.write(std::vector<std::vector<int>>{{1, 2}, {3, 4}}); }) == "1 2\n3 4");
    REQUIRE(written([](cp::io::writer& w) { w.write(std::vector<std::string>{"x", "y"}); }) == "x y");
    REQUIRE(written([](cp::io::writer& w) { w.write(std::pair{1, 'x'}, record{1, "n", {2, 3}, {'t', 0.5}}); }) ==
            "1 x 1 n 2 3 t 0.500000000");
    REQUIRE(written([](cp::io::writer& w) { w.write(user::point{1, 2}, std::vector<bool>{true, false}); }) ==
            "(1,2) 1 0");
    REQUIRE(written([](cp::io::writer& w) {
                w.write(std::numeric_limits<cp::num::i128>::min(), std::uint8_t{200}, std::int8_t{-5});
            }) == "-170141183460469231731687303715884105728 200 -5");
    REQUIRE(written([](cp::io::writer& w) {
                w.write(cp::io::arg_sep(", ") | cp::io::range_sep("|"), std::vector{1, 2}, 3);
            }) == "1|2, 3");
}

TEST_CASE("formatters govern the arguments after them, or the values they wrap", "[io][format]") {
    using namespace cp::io;
    REQUIRE(written([](writer& w) { w.write(width(4), -5, 7); }) == "  -5    7");
    REQUIRE(written([](writer& w) { w.write(width(4)(-5), 7); }) == "  -5 7");
    REQUIRE(written([](writer& w) { w.write(width(4) | fill('0'), -5, 7); }) == "-005 0007");
    REQUIRE(written([](writer& w) { w.write(width(2), 12345); }) == "12345");
    REQUIRE(written([](writer& w) { w.write(show_plus(), 7, -7, 0); }) == "+7 -7 +0");
    REQUIRE(written([](writer& w) { w.write(base(16), 255, -255); }) == "ff -ff");
    REQUIRE(written([](writer& w) { w.write(precision(3) | precision(2), 3.14159); }) == "3.14");  // right wins
    REQUIRE(written([](writer& w) { w.write(float_format(std::chars_format::general) | precision(-1), 0.1, 1e300); }) ==
            "0.1 1e+300");
    REQUIRE(written([](writer& w) { w.write(show_bool_words(), true, false); }) == "true false");
    REQUIRE(written([](writer& w) { w.write(precision(2), 1.0, defaults(), 1.0); }) == "1.00 1.000000000");
    REQUIRE(written([](writer& w) { w.write(precision(2), 1.0, show_bool_words(), 1.0, true); }) ==
            "1.00 1.000000000 true");  // replace, not layer
    REQUIRE(written([](writer& w) { w.write(precision(1) | show_bool_words(), 0.25, true); }) == "0.2 true");
    REQUIRE(written([](writer& w) { w.write(arg_sep(","), 1, 2, defaults(), 3); }) == "1,2 3");
    REQUIRE(written([](writer& w) { w.write(precision(1), 1.0, precision(3)(2.0), 3.0); }) == "1.0 2.000 3.0");
    REQUIRE(written([](writer& w) { w.write(precision(2), 1.0).write(1.0); }) == "1.001.000000000");  // per call
    REQUIRE(written([](writer& w) { w.write(precision(2), user::point{1, 2}, 0.5); }) == "(1,2) 0.50");
}

TEST_CASE("layout follows nesting levels", "[io][format]") {
    using namespace cp::io;
    std::map<int, int> m{{1, 10}, {2, 20}};
    std::map<int, std::vector<int>> mv{{1, {5, 6}}, {2, {7}}};
    std::vector<std::vector<std::vector<int>>> c{{{1, 2}, {3, 4}}, {{5, 6}}};
    REQUIRE(written([&](writer& w) { w.write(m); }) == "1 10\n2 20");
    REQUIRE(written([&](writer& w) { w.write(std::vector<std::pair<int, int>>{{1, 2}, {3, 4}}); }) == "1 2\n3 4");
    REQUIRE(written([&](writer& w) { w.write(mv); }) == "1 5 6\n2 7");
    REQUIRE(written([&](writer& w) { w.write(c); }) == "1 2\n3 4\n5 6");
    REQUIRE(written([&](writer& w) { w.write(range_sep(" ", "\n", "\n\n")(c)); }) == "1 2\n3 4\n\n5 6");
    auto py = range_sep(", ") | range_bracket("[", "]") | field_sep(", ") | field_bracket("(", ")");
    REQUIRE(written([&](writer& w) { w.write(py(m)); }) == "[(1, 10), (2, 20)]");
    REQUIRE(written([&](writer& w) { w.write(py(mv)); }) == "[(1, [5, 6]), (2, [7])]");
    REQUIRE(written([&](writer& w) { w.write(py(std::array<int, 2>{1, 2}), std::vector<bool>{true, false}); }) ==
            "[1, 2] 1 0");
    REQUIRE(written([&](writer& w) { w.write(py(std::vector<std::string>{"a b", "c"})); }) == "[a b, c]");
    REQUIRE(written([&](writer& w) { w.write(field_sep(",")(std::pair{1, 2}), std::pair{3, 4}); }) == "1,2 3 4");
}

namespace {
struct edge {
    int u, v;
    long long w;
};
}  // namespace

TEST_CASE("quote and field names", "[io][format]") {
    using namespace cp::io;
    REQUIRE(written([](writer& w) { w.write(show_quotes()(std::string("a \"b\"\n"), 'c', '\'', "t\\")); }) ==
            R"("a \"b\"\n" 'c' '\'' "t\\")");
    REQUIRE(written([](writer& w) { w.write(show_quotes()(std::vector<std::string>{"x"}), "y"); }) == "\"x\" y");
    auto named =
        written([](writer& w) { w.write((show_field_names() | field_bracket("{", "}") | field_sep(", "))(edge{1, 2, 5})); });
    if constexpr (cp::detail::has_field_names_v<edge>) {
        REQUIRE(cp::detail::field_names_v<edge>[1] == "v");
        REQUIRE(named == "{u: 1, v: 2, w: 5}");
    } else {
        REQUIRE(named == "{1, 2, 5}");
    }
#if defined(__GNUC__) && !defined(__clang__) && __cplusplus >= 202002L
    REQUIRE(cp::detail::has_field_names_v<edge>);  // the C++20 path is really taken where it should be
#endif
    REQUIRE(written([](writer& w) { w.write(show_field_names()(std::pair{1, 2})); }) == "1 2");
}

TEST_CASE("wrap breaks wide values pprint style", "[io][format]") {
    using namespace cp::io;
    auto py = range_sep(", ") | range_bracket("[", "]");
    std::vector<std::vector<int>> g{{1, 2, 3}, {4, 5, 6}};
    REQUIRE(written([&](writer& w) { w.write(py | wrap(80), g); }) == "[[1, 2, 3], [4, 5, 6]]");
    REQUIRE(written([&](writer& w) { w.write(py | wrap(12), g); }) == "[\n  [1, 2, 3],\n  [4, 5, 6]\n]");
    REQUIRE(written([&](writer& w) { w.write(py | wrap(5), g); }) ==
            "[\n  [\n    1,\n    2,\n    3\n  ],\n  [\n    4,\n    5,\n    6\n  ]\n]");
    REQUIRE(written([&](writer& w) { w.write(wrap(3), std::vector{10, 20}); }) == "\n  10\n  20\n");
    std::vector<std::vector<long long>> big(30, std::vector<long long>(40, -123456789012345LL));
    auto const write_big = [&](writer& w) { w.write("head", py | wrap(100), big, 1e300); };
    REQUIRE(written(write_big, 256) == written(write_big));  // measuring through a buffer smaller than the value
}

namespace {
struct flags {
    unsigned a : 4, b : 4;
};
struct node {
    int v;
    std::vector<node> children;
};
}  // namespace

TEST_CASE("bit-field and recursive structs write", "[io][format]") {
    using namespace cp::io;
    REQUIRE(written([](writer& w) { w.write(flags{1, 2}, show_field_names()(flags{3, 4})); }) == "1 2 3 4");
    node tree{1, {{2, {}}, {3, {}}}};
    auto const py = range_sep(", ") | range_bracket("[", "]") | field_sep(", ") | field_bracket("{", "}");
    REQUIRE(written([&](writer& w) { w.write(py, tree); }) == "{1, [{2, []}, {3, []}]}");
}

TEST_CASE("a small writer buffer flushes correctly", "[io][stream]") {
    std::string expected, big(1000, 'z');
    for (int i = 0; i < 20000; ++i) expected += std::to_string(i * 7919 - 50000) + '\n';
    expected += big;
    auto got  = written(
        [&](cp::io::writer& w) {
            for (int i = 0; i < 20000; ++i) w.writeln(i * 7919 - 50000);
            w.write(big);
        },
        256);
    REQUIRE(got == expected);
    // Fixed doubles near the maximum need more than 256 bytes.
    auto huge = written([](cp::io::writer& w) { w.write(1e300); }, 256);
    REQUIRE(huge.size() == 301 + 10);
}

namespace {
// The largest struct the automatic path handles before C++26 reflection: every generated case up to 32 fields.
struct wide_record {
    int f00, f01, f02, f03, f04, f05, f06, f07, f08, f09, f10, f11, f12, f13, f14, f15;
    int f16, f17, f18, f19, f20, f21, f22, f23, f24, f25, f26, f27, f28, f29, f30, f31;
};
}  // namespace

TEST_CASE("structs of up to 32 fields read and write in field order", "[io][stream]") {
    std::string text;
    for (int i = 0; i < 32; ++i) text += std::to_string(i * 3) + ' ';
    file_input f(text);
    wide_record r{};
    REQUIRE(f.in.read(r));
    REQUIRE((r.f00 == 0 && r.f13 == 39 && r.f26 == 78 && r.f31 == 93));
    text.pop_back();
    REQUIRE(written([&](cp::io::writer& w) { w.write(r); }) == text);
}
