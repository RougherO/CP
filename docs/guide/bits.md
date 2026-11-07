# bits: bit operations

`#include "cp/bits/bits.hpp"`: bit operations on unsigned integers of any width, `unsigned __int128` and cp's wider
integers included.

<!-- example: bits.cpp -->
```cpp
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
```

| Function | Result |
|---|---|
| `bits::popcnt(x)` | number of 1 bits |
| `bits::clz(x)`, `bits::ctz(x)` | leading / trailing zero bits |
| `bits::clo(x)`, `bits::cto(x)` | leading / trailing one bits |
| `bits::width(x)` | bits needed to hold `x` (`0` for `0`) |
| `bits::floor2(x)`, `bits::ceil2(x)` | largest power of two `<= x`, smallest `>= x` |
| `bits::is_pow2(x)` | whether `x` is a power of two |
| `bits::rotl(x, n)`, `bits::rotr(x, n)` | rotation |
| `bits::bswap(x)` | bytes reversed |
| `bits::cast<To>(x)` | the same bits as another type of the same size |
| `bits::endian` | the platform's byte order |

Unsigned types only, as in `<bit>`: `bits::clz(-1)` does not compile; cast first. The standard function does the
work when the standard has it (`std::countl_zero` behind `bits::clz`, ...), otherwise a compiler builtin.
