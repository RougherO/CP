// Formatters: how values are read and written, per call. Nothing carries over to the next call.
#include "cp/io/io.hpp"
#include <map>
#include <vector>
using namespace cp;

int main() {
    double x = 3.14159265358979;
    io::writeln(io::precision(12), x);                // 3.141592653590: a formatter governs the arguments after it
    io::writeln(io::precision(2), x, io::defaults(), x);  // 3.14 3.141592654: io::defaults() is back to the defaults

    std::vector<int> v{1, 2, 3};
    io::writeln("v:", io::range_sep(",")(v), 4);  // v: 1,2,3 4 (calling a formatter decorates only its values)

    constexpr auto py = io::range_sep(", ") | io::range_bracket("[", "]") | io::field_sep(", ") |
                        io::field_bracket("(", ")");        // | combines formatters; keep presets as constants
    io::writeln(py, std::map<int, int>{{1, 10}, {2, 20}});  // [(1, 10), (2, 20)]

    long long h = 0;
    io::read(io::base(16), h);                                    // read in hexadecimal
    io::writeln(io::base(16) | io::width(8) | io::fill('0'), h);  // written back as 8 hex digits
}
