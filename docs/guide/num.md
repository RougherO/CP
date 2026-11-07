# num: integers

`#include "cp/num/num.hpp"`: 128-bit and wider integers, exact and saturating arithmetic, comparison across
signedness, integers modulo M, and constants.

<!-- example: num.cpp -->
```cpp
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
```

## Types

| Name | What it is |
|---|---|
| `num::i128`, `num::u128` | 128-bit integers: the compiler's `__int128` when it has one, otherwise cp's own |
| `num::widen_t<T>` | the integer type twice as wide as `T`: `u64` → `u128` → 256 bits → 512 bits … |
| `num::wide<T>` | a double-width result in two halves: `low`, `high`, `overflow()`, `widen()` |
| `num::strong_ordering`, `weak_ordering`, `partial_ordering` | what `num::cmp` returns (the std types from C++20) |

Literals for constants wider than 64 bits: `123_i128`, `0xffff'ffff'ffff'ffff'ffff_u128` (after
`using namespace cp::aliases;  // the _u128 literal`). A literal that does not fit is a compile error.

`i128` and `u128` read and write with cp's I/O; standard streams cannot print `__int128`. Cast with
`static_cast<u64>(x)` to narrow: it works for both implementations.

## Functions

| Function | Result |
|---|---|
| `num::add_wide(a, b)`, `sub_wide`, `mul_wide` | the exact result as `wide<T>`: you decide whether overflow wraps, saturates or widens |
| `num::add_sat(a, b)`, `sub_sat`, `mul_sat`, `div_sat` | the result clamped to the type's range instead of wrapping |
| `num::cast_sat<T>(x)` | `x` converted to `T`, clamped |
| `num::midpoint(a, b)` | `(a + b) / 2` without overflow, rounded towards `a` |
| `num::cmp(a, b)` | three-way comparison by value: `num::cmp(-1, 0U) < 0` is true (the plain `-1 < 0U` is false); floating-point compares as `partial_ordering` (NaN unordered) |
| `num::fits<T>(x)` | whether `x` fits in `T` |

Saturating functions and `cmp` take integers of any width, `__int128` and wider ones included (not `bool` or
characters); both operands of a saturating function have one type. Mixing an integer and a floating-point value in
`cmp` does not compile.

## Notes

- Above 128 bits the arithmetic is cp's own: every doubling makes `*` about four times slower.
- Signed overflow wraps in cp's integers and is undefined for `__int128`; rely on neither.
- With checks on, division by zero and the one overflowing division (`min / -1`) are reported.

## Modular integers

<!-- example: modint.cpp -->
```cpp
// Arithmetic modulo a prime or any other modulus, and the constants that come with it.
#include "cp/io/io.hpp"
#include "cp/num/num.hpp"
using namespace cp;
using cp::num::consts::inf, cp::num::consts::mod1e9_7, cp::num::consts::pi, cp::num::consts::tau;
using mint = num::modint998;                  // modulo 998244353; num::modint<M> for any other M

int main() {
    mint a(-1), b(10);                        // explicit: a is 998244352
    io::writeln(a + b, a * b, b / 3 * 3 == b, b.pow(-1) * b);   // 9 998244343 1 1

    mint x;
    io::read(x);                              // any integer, reduced: -5 reads as 998244348
    io::writeln(x, x.inv() * x);

    num::dyn_modint<>::set_mod(*io::read<int>());   // a modulus from the input
    num::dyn_modint<> y(100);
    io::writeln(y, y * y);

    io::writeln(inf, mod1e9_7, pi, tau);
}
```

| Name | What it is |
|---|---|
| `num::modint<M>` | an integer modulo `M`, fixed at compile time; any `M` from 1 to 2⁶⁴ − 1 |
| `num::modint998`, `num::modint1e9_7` | `modint<998244353>`, `modint<1000000007>` |
| `num::dyn_modint<Id>` | an integer modulo a run-time modulus below 2³², set with `dyn_modint<Id>::set_mod(m)`; each `Id` has its own |

| Operation | Result |
|---|---|
| `modint<M>(x)` | `x mod M` in [0, M) for any integer, negatives and 128-bit ones included; explicit |
| `+ - * /`, `+= -= *= /=`, unary `-`, `++`, `--` | modulo M; the other operand may be an integer: `x * 2`, `1 - x` |
| `==`, `!=` | same residue; `x == 0` works |
| `x.pow(e)` | xᵉ by squaring, O(log e); a negative `e` raises the inverse |
| `x.inv()`, `a / b` | the inverse by the extended Euclidean algorithm: any modulus, prime or not, as long as gcd(x, M) = 1 |
| `x.value()`, `T::mod()` | the residue, the modulus |

They read and write with cp's I/O and work as `std::unordered_map` keys. There is no `<` and no bitwise
operator: they mean nothing modulo M.

- With checks on, `inv()` or `/` of a value that shares a factor with M is reported (for example `2` modulo
  `1000`); without checks the result is meaningless.
- Values above 2³² − 1 are stored in 64 bits and multiply through a 128-bit product: about half the speed.
- `dyn_modint` starts at 998244353. Set the modulus before making values: a value made earlier keeps its old
  residue.

Multiplications per second (`xmake bench`, GCC -O2, Intel i5-12600K):

| | independent | each waits for the previous |
|---|---|---|
| `a * b % 998244353` by hand | 1.61 G | 411 M |
| `num::modint998` | 1.57 G | 387 M |
| `a * b % m`, `m` read at run time | 1.62 G | 381 M |
| `num::dyn_modint<>` | 1.57 G | 334 M |
| `num::modint<(1 << 61) - 1>` | 931 M | 221 M |

`modint<M>` costs what the hand-written `%` costs: with `M` known, the compiler replaces the division by a
multiplication. `dyn_modint` multiplies by a precomputed 2⁶⁴/m instead of dividing (Barrett reduction). This
processor divides unusually fast (10–15 cycles for 64 bits); on the older processors many judges run, a division
takes 35–90 cycles and Barrett reduction is several times faster than `%`.

## Constants

`num::consts::pi`, or `using namespace cp::num::consts;` (not `numbers`: that would be ambiguous with
`std::numbers` after `using namespace std;`). Opening the namespace brings in short names such as `e` and `phi`;
with `-Wshadow`, a variable `e` then draws a warning. Name the constants you use instead:
`using cp::num::consts::pi;`.

| Name | Value |
|---|---|
| `e`, `pi`, `inv_pi`, `inv_sqrtpi`, `sqrt2`, `sqrt3`, `inv_sqrt3`, `ln2`, `ln10`, `log2e`, `log10e`, `egamma`, `phi` | `std::numbers`' constants, `double`; `pi_v<T>` and so on for `float` or `long double`. From C++20 they are `std::numbers`' own |
| `tau`, `tau_v<T>` | 2π |
| `inf`, `inf_v<T>` | integers: `max() / 2`, so `inf + inf` does not overflow (`inf` is the `int64_t` one, 4.6·10¹⁸); floating point: infinity |
| `mod998`, `mod1e9_7`, `mod1e9_9` | 998244353, 10⁹ + 7, 10⁹ + 9 |
| `mersenne61` | 2⁶¹ − 1, prime: the modulus for rolling hashes |
| `ntt_root` | 3, a primitive root modulo 998244353 |

The integer constants are `int64_t`, so `a * b % mod1e9_7` stays signed and does not overflow for `a, b` below
the modulus.
