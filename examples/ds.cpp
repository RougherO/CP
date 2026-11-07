// span, static_vector and ssize.
#include "cp/ds/ds.hpp"
#include "cp/io/io.hpp"
#include <vector>
using namespace cp;

int main() {
    std::vector<int> v{5, 1, 4};
    ds::span<int> s(v);                       // a view of v's elements
    io::writeln(s.size(), s.first(2), ds::ssize(v) - 4);

    ds::static_vector<int, 4> small{3, 1, 2}; // at most 4 ints, stored inline: no allocation
    small.push_back(7);
    erase(small, 1);                          // found by argument-dependent lookup
    io::writeln(small, small.size(), small.capacity());
}
