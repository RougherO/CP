// CP_CHECK, cp::debug::check and cp::io::dump, run in a child process whose standard streams are pipes.
#include "cp/debug/debug.hpp"
#include "cp/io/io.hpp"
#include "fd_helpers.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <type_traits>
#include <vector>

#if CP_OS_POSIX || CP_OS_WINDOWS
using namespace test;  // NOLINT(google-build-using-namespace): test helpers

namespace {
int calls = 0;
auto counted() -> bool { return ++calls > 0; }
}  // namespace

TEST_CASE("CP_CHECK evaluates its condition only under CP_CHECKS; io::dump always evaluates", "[debug]") {
    calls = 0;
    CP_CHECK(counted());
    CP_CHECK(std::is_same_v<int, signed>);  // commas inside the condition
    REQUIRE(calls == CP_CHECKS);
    auto r = run_child("", [] {
        calls = 0;  // the child starts with the parent's count
        cp::io::dump(counted());
        cp::io::write(calls);
        cp::io::out.flush();
    });
    REQUIRE(r.out == "1");
}

#  if CP_CHECKS
TEST_CASE("a failed check keeps cp::io::out's output and reports the caller's line", "[debug]") {
    int const check_line = __LINE__ + 3;
    auto r               = run_child("", [] {
        cp::io::write("kept");
        CP_CHECK(1 + 1 == 3);
    });
    CAPTURE(r.err);
    REQUIRE(r.aborted);
    REQUIRE(r.out == "kept");
    REQUIRE(r.err.find("test_debug.cpp:" + std::to_string(check_line)) != std::string::npos);
    REQUIRE(r.err.find("check failed: 1 + 1 == 3\n") != std::string::npos);

    auto eof = run_child("x", [] {  // a library report: a bad token
        cp::io::write("kept");
        static_cast<void>(cp::io::read<int>());
    });
    REQUIRE(eof.aborted);
    REQUIRE(eof.out == "kept");
}

TEST_CASE("io::dump prints the values pretty and the caller's line", "[debug]") {
    int const dump_line = __LINE__ + 5;
    auto r              = run_child("", [] {
        int x = 7;
        std::vector<int> v{1, 2};
        cp::io::write("out first");
        cp::io::dump(x);
        cp::io::dump(x + 1, v, "s");
        cp::io::dump();
        cp::io::dump(std::pair{1, 'c'}, cp::io::float_format(std::chars_format::fixed) | cp::io::precision(3), 0.5,
                     true);
    });
    auto at             = [&](int i) { return "[test_debug.cpp:" + std::to_string(dump_line + i) + "]"; };
    REQUIRE(r.out == "out first");
    REQUIRE(r.err ==
            at(0) + " 7\n" + at(1) + " 8 | [1, 2] | \"s\"\n" + at(2) + "\n" + at(3) + " {1, 'c'} | 0.500 | true\n");
}
#  endif
#endif
