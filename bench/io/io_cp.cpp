// cp::io::read / cp::io::writeln on the standard streams.
#include "common.hpp"
#include "cp/io/io.hpp"
#include <cstdio>

int main(int argc, char** argv) {
    if (iobench::reading(argc, argv)) {
        long long n = 0, x = 0, sum = 0;
        cp::io::read(n);
        for (long long i = 0; i < n; ++i) cp::io::read(x), sum += x;
        std::fprintf(stderr, "%lld\n", sum);
    } else {
        auto n          = iobench::count(argc, argv);
        std::uint64_t s = 1;
        cp::io::writeln(n);
        for (long long i = 0; i < n; ++i) cp::io::writeln(iobench::next_value(s));
    }
}
