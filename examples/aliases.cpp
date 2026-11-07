// Short integer names and literals, opt-in.
#include "cp/aliases/aliases.hpp"
#include "cp/io/io.hpp"
#include <vector>
using namespace cp;
using namespace cp::aliases;

int main() {
    i64 big = 1'000'000'000'000_i64;
    u32 mask = 0xff;
    usize n = 3_uz;
    std::vector<i64> v(n, big);
    io::writeln(v, mask, sizeof(u16), static_cast<i64>(-5_iz));
}
