# Language levels

cp works at C++17, 20, 23 and 26 with the same names. When the standard library has a facility, cp's name is that
facility or a call to it; otherwise cp provides its own with the same behaviour.

| cp name | From the standard library at | Before that |
|---|---|---|
| `ds::span` | C++20 (`std::span`) | cp's backport |
| `ds::ssize` | C++20 (`std::ssize`) | cp's |
| `ds::static_vector` | C++26 (`std::inplace_vector`) | cp's |
| `views::iota`, `views::reverse` | C++20 (all of `std::views`) | cp's two views |
| `bits::*` | C++20 (`<bit>`: `std::popcount` behind `bits::popcnt`, ...) | compiler builtins |
| `num::cmp`, orderings | C++20 (`std::cmp_*`, `<compare>`) | cp's |
| `num::add_sat`, ... | C++26 (`std::saturating_add`, ...) | cp's |
| `num::midpoint` | C++20 (`std::midpoint`) | cp's |
| `num::i128`, `num::u128` | the compiler's `__int128` | cp's 128-bit type |
| `util::result`, `util::failure` | C++23 (`std::expected`, `std::unexpected`) | cp's |
| `traits::remove_cvref_t`, `type_identity_t`, `is_constant_evaluated` | C++20 | cp's |
| `traits::to_underlying`, `debug::unreachable` | C++23 | cp's |
| `debug::source_location` | C++20 | compiler builtins |
| plain-struct I/O | C++26 reflection (GCC 16 `-freflection`): no field limit | up to 32 fields |

## `using namespace std;` with cp

Open both freely:

```cpp
using namespace std;
using namespace cp;
using namespace cp::io;   // or any other module
```

A cp name equals a std name only when it *is* that std entity, so nothing becomes ambiguous. The test
`test_both_namespaces` opens `std` and every module and uses each name unqualified, at every language level.

## The oldest standard

cp needs C++17 or later, GCC or Clang (MinGW-w64 GCC on Windows) and a 64-bit x86-64 or AArch64 target; anything
else stops with a message. MSVC is not supported.
