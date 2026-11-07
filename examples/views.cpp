// views::iota and views::reverse, also in C++17; binary search over a range of numbers.
#include "cp/io/io.hpp"
#include "cp/views/views.hpp"
#include <algorithm>
#include <vector>
using namespace cp;

int main() {
    long long sum = 0;
    for (int i : views::iota(1, 11)) sum += i;
    std::vector<int> v{1, 2, 3}, backwards;
    for (int x : views::reverse(v)) backwards.push_back(x);

    auto r       = views::iota(0LL, 1'000'000'000LL);                 // the smallest x with x * x >= 10^12
    auto first_x = *std::partition_point(r.begin(), r.end(), [](long long x) { return x * x < 1'000'000'000'000LL; });
    io::writeln(sum, backwards, first_x);     // 55 3 2 1 1000000
}
