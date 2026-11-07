// Every way to read and write with cp::io.
#include "cp/io/io.hpp"
#include <string>
#include <tuple>
#include <utility>
#include <vector>
using namespace cp;

int main() {
    int n = 0;
    std::string name;
    io::read(n, name);  // into variables, like std::cin >> n >> name

    std::vector<long long> a(n);
    io::read(a);  // a range reads as many elements as it holds

    auto k      = *io::read<int>();       // declare and read
    auto [h, w] = *io::read<int, int>();  // several values, as a tuple

    std::vector<std::pair<int, int>> edges(k);
    io::read(edges);  // pairs, tuples and plain structs read field by field

    std::string line;
    io::getline(line);                        // the rest of the current line
    io::getline(line);                        // the next line
    io::readln(n);                            // n, then the rest of its line is dropped
    auto hex = *io::read<int>(io::base(16));  // with a formatter (see io_options.cpp)

    long long x = 0, sum = 0;
    while (io::read(x)) sum += x;  // until end of input

    io::write(n, name);  // separated by a space, no newline
    io::writeln();       // just a newline
    io::writeln(a);      // a range: elements separated by spaces
    io::writeln(h * w, edges.size(), sum, line, hex);
    io::writeln(std::vector<std::vector<int>>{{1, 2}, {3, 4}});  // nested ranges: one row per line
    io::writeln(edges);                                          // a range of pairs: one pair per line
}
