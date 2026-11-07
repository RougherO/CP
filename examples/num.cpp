// 128-bit and wider integers, exact and saturating arithmetic, comparison across signedness.
#include "cp/aliases/aliases.hpp"
#include "cp/io/io.hpp"
#include "cp/num/num.hpp"
#include <cstdint>
using namespace cp;
using namespace cp::aliases;  // the _u128 literal

int main() {
    num::u128 big = 340282366920938463463374607431768211455_u128;   // the largest u128
    num::i128 product = num::i128{1'000'000'007} * 998'244'353;
    io::writeln(big, product);

    auto wide = num::mul_wide(~std::uint64_t{0}, std::uint64_t{3});  // the full 128-bit product, in halves
    io::writeln(wide.high, wide.low, wide.overflow());

    num::widen_t<num::u128> huge = big;      // 256 bits
    io::writeln(huge * huge > huge);

    io::writeln(num::add_sat(std::int8_t{100}, std::int8_t{100}));   // 127, not wrapped
    io::writeln(num::cmp(-1, 0U) < 0, num::fits<std::uint8_t>(300), num::midpoint(2, 9));
}
