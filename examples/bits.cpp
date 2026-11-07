// Bit operations on unsigned integers.
#include "cp/bits/bits.hpp"
#include "cp/io/io.hpp"
#include <cstdint>
using namespace cp;

int main() {
    std::uint32_t x = 40;                     // 101000
    io::writeln(bits::popcnt(x), bits::clz(x), bits::ctz(x), bits::width(x));  // 2 26 3 6
    io::writeln(bits::floor2(x), bits::ceil2(x), bits::is_pow2(x));            // 32 64 0
    io::writeln(bits::rotl(x, 1), bits::bswap(std::uint16_t{0x0102}));         // 80 513
}
