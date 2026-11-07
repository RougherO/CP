// cp::io::in, cp::io::out, cp::io::err and the free functions, run in a child process whose standard streams are pipes.
#include "cp/io/io.hpp"
#include "fd_helpers.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string>
#include <vector>

#if CP_OS_POSIX || CP_OS_WINDOWS
using namespace test;  // NOLINT(google-build-using-namespace): test helpers

TEST_CASE("the free functions read and write the standard streams", "[io]") {
    auto r = run_child("3\n1 2 3\n4 5\n", [] {
        int n = 0;
        cp::io::read(n);
        std::vector<long long> a(static_cast<std::size_t>(n));
        cp::io::read(a);
        auto [x, y] = *cp::io::read<int, int>();
        cp::io::writeln(n, a, x + y);
        cp::io::out.flush();
    });
    REQUIRE_FALSE(r.aborted);
    REQUIRE(r.out == "3 1 2 3 9\n");
}

TEST_CASE("a read loop ends at end of input, and getline reads lines", "[io]") {
    auto r = run_child("1 2\n3", [] {
        long long x = 0, sum = 0;
        while (cp::io::read(x)) sum += x;
        cp::io::writeln(sum, cp::io::in.eof());
        cp::io::out.flush();
    });
    REQUIRE(r.out == "6 1\n");
    auto g = run_child("a b\r\nc", [] {
        std::string s, t;
        cp::io::getline(s);
        cp::io::getline(t);
        cp::io::write(s, '|', t);
        cp::io::out.flush();
    });
    REQUIRE(g.out == "a b | c");
}

TEST_CASE("free readln, getline and formatter arguments on cp::io::in", "[io]") {
    auto r = run_child("ff 10 junk\nwhole line\n7", [] {
        int h = 0, d = 0;
        std::string line;
        cp::io::readln(cp::io::base(16), h, cp::io::with(), d);
        cp::io::getline(line);
        auto k = cp::io::read<int>(cp::io::base(8));
        cp::io::write(h, d, line, *k);
        cp::io::out.flush();
    });
    REQUIRE_FALSE(r.aborted);
    REQUIRE(r.out == "255 10 whole line 7");
}

TEST_CASE("cp::io::err writes through", "[io]") {
    auto r = run_child("", [] {
        cp::io::err.write("before the crash");
        std::abort();
    });
    REQUIRE(r.aborted);
    REQUIRE(r.err == "before the crash");
}

#  if CP_CHECKS
TEST_CASE("errors point at the cp::io::read call that made them", "[io]") {
    int const read_line = __LINE__ + 3;
    auto bad            = run_child("x", [] {
        int n = 0;
        cp::io::read(n);
    });
    CAPTURE(bad.err);
    REQUIRE(bad.aborted);
    REQUIRE(bad.err.find("test_io.cpp:" + std::to_string(read_line)) != std::string::npos);
    REQUIRE(bad.err.find("input line 1: invalid token \"x\"") != std::string::npos);

    int const value_line = __LINE__ + 1;
    auto big             = run_child("1\n300", [] { static_cast<void>(*cp::io::read<int, std::uint8_t>()); });
    CAPTURE(big.err);
    REQUIRE(big.aborted);
    REQUIRE(big.err.find("test_io.cpp:" + std::to_string(value_line)) != std::string::npos);
    REQUIRE(big.err.find("input line 2: out-of-range number \"300\"") != std::string::npos);
}
#  endif

TEST_CASE("read<T>() returns the value or the error; end of input is returned, never reported", "[io]") {
    auto r = run_child("5 6", [] {
        long long sum = 0;
        while (auto x = cp::io::read<long long>()) sum += *x;
        auto end = cp::io::read<int>();
        cp::io::write(sum, end.has_value(), end.error() == cp::io::read_error::end_of_input);
        cp::io::out.flush();
    });
    REQUIRE_FALSE(r.aborted);
    REQUIRE(r.out == "11 0 1");
}

// A judge that answers each query only after it arrives: without the tie flush the solution's query would sit in
// its buffer and both sides would wait forever.
TEST_CASE("interactive: queries reach the judge without a manual flush", "[io]") {
    deadline const limit(30);  // a deadlock fails the test instead of hanging it
    auto c           = spawn([] {
        int lo = 1, hi = 1'000'000;
        while (lo < hi) {
            int mid = lo + (hi - lo) / 2;
            cp::io::writeln('?', mid);
            int at_most = 0;
            cp::io::read(at_most);
            if (at_most != 0)
                hi = mid;
            else
                lo = mid + 1;
        }
        cp::io::writeln('!', lo);
        cp::io::out.flush();
    });
    int const secret = 777'777;
    cp::io::writer to_solution(c->in.write_end());
    cp::io::reader from_solution(c->out.read_end(), 64, &to_solution);
    int queries = 0, answer = 0;
    for (char kind = 0; from_solution.read(kind) && kind == '?';) {
        int m = 0;
        from_solution.read(m);
        to_solution.writeln(secret <= m ? 1 : 0);
        ++queries;
    }
    from_solution.read(answer);
    auto r = finish(*c);
    REQUIRE(answer == secret);
    REQUIRE(queries <= 20);
    REQUIRE_FALSE(r.aborted);
}
#endif
