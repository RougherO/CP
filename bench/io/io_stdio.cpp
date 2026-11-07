// scanf / printf.
#include "common.hpp"
#include <cstdio>

int main(int argc, char** argv) {
    if (iobench::reading(argc, argv)) {
        long long n = 0, x = 0, sum = 0;
        if (std::scanf("%lld", &n) != 1) return 1;
        for (long long i = 0; i < n && std::scanf("%lld", &x) == 1; ++i) sum += x;
        std::fprintf(stderr, "%lld\n", sum);
    } else {
        auto n          = iobench::count(argc, argv);
        std::uint64_t s = 1;
        std::printf("%lld\n", n);
        for (long long i = 0; i < n; ++i) std::printf("%lld\n", iobench::next_value(s));
    }
}
