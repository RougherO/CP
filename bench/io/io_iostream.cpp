// std::cin / std::cout, unsynced and untied: the usual contest setup.
#include "common.hpp"
#include <cstdio>
#include <iostream>

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);
    if (iobench::reading(argc, argv)) {
        long long n = 0, x = 0, sum = 0;
        std::cin >> n;
        for (long long i = 0; i < n; ++i) std::cin >> x, sum += x;
        std::fprintf(stderr, "%lld\n", sum);
    } else {
        auto n          = iobench::count(argc, argv);
        std::uint64_t s = 1;
        std::cout << n << '\n';
        for (long long i = 0; i < n; ++i) std::cout << iobench::next_value(s) << '\n';
    }
}
