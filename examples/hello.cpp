// Sum of each test case's numbers: the shape of most solutions.
#include "cp/io/io.hpp"
#include <vector>
using namespace cp;

int main() {
    for (auto t = *io::read<int>(); t--;) {
        auto n = *io::read<int>();
        std::vector<long long> a(n);
        io::read(a);
        long long sum = 0;
        for (auto x : a) sum += x;
        io::writeln(sum);
    }
}
