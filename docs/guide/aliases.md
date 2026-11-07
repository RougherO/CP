# aliases: short integer names

`#include "cp/aliases/aliases.hpp"`, then `using namespace cp::aliases;`. Opt-in: `cp/all.hpp` does not include
it and the library never uses it.

<!-- example: aliases.cpp -->
```cpp
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
```

| Names | Types |
|---|---|
| `i8`, `i16`, `i32`, `i64` | `std::int8_t` … `std::int64_t` |
| `u8`, `u16`, `u32`, `u64` | `std::uint8_t` … `std::uint64_t` |
| `i128`, `u128` | `cp::num::i128`, `cp::num::u128` (see [num](num.md)) |
| `isize`, `usize` | `std::ptrdiff_t`, `std::size_t` |
| `_i32`, `_i64`, `_i128`, `_u32`, `_u64`, `_u128`, `_iz`, `_uz` | literals: `5_i64`, `1_u128`, `3_uz` |

A literal that does not fit its type does not compile. A literal is never negative: `-5_i64` is `-(5_i64)`.
There are no `_i8`, `_i16`, `_u8`, `_u16`: any arithmetic on those types gives an `int`.
