// Checks and debug output: active when CP_CHECKS is 1 (xmake's debug mode), gone in submissions.
#include "cp/debug/debug.hpp"
#include "cp/io/io.hpp"
#include <vector>
using namespace cp;

int main() {
    auto n = *io::read<int>();
    std::vector<int> a(n);
    io::read(a);
    CP_CHECK(n > 0);  // stops with file:line and "check failed: n > 0"
    io::dump(n, a);   // [debugging.cpp:12] 3 | [1, 2, 3]   (on stderr)
    io::dump();       // [debugging.cpp:13]                 (reached here)
    io::writeln(a.back());
}
