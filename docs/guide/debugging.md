# Checks and debugging

`#include "cp/debug/debug.hpp"` for checks; `io::dump` comes with `cp/io/io.hpp`. Everything here is active only
when `CP_CHECKS` is 1: xmake's debug mode (the default) sets it, and `xmake bundle --checks=1` keeps it in a
submission. Otherwise it compiles to nothing; `CP_CHECK`'s condition is not evaluated, `io::dump`'s arguments are.

<!-- example: debugging.cpp -->
```cpp
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
```

## `CP_CHECK(condition)`

Stops the program when `condition` is false, after flushing `io::out` so the output so far is kept:

```
main.cpp:11: int main(): check failed: n > 0
```

Commas inside the condition are fine: `CP_CHECK(std::is_same_v<A, B>)`.

## `io::dump(values...)`

Prints the values to standard error with the file and line, laid out for reading:

```
[main.cpp:12] 3 | [1, 2, 3] | {u: 1, v: 2, w: 5} | "text" | true
```

`|` between values, `[…]` around ranges, `{…}` around pairs and structs (with field names from C++20), quoted
strings, `true`/`false`, the shortest exact floats, and anything wider than 80 characters broken one element per line.
That look is `io::dump_style`; formatter arguments apply on top of it, as in any call:
`io::dump(io::precision(3), d)`. `io::dump()` prints just the place: the program got there. `io::out` is flushed
first, so on a terminal the line appears after the output before it.

It is a function, so with checks off its arguments are still evaluated: keep side effects out of them.

## What a checked build catches

| Mistake | Report |
|---|---|
| a token that is not a number, or does not fit | `cp::io::read: input line 3: invalid token "x"` at your `io::read` line |
| `CP_CHECK` false | `check failed: <condition>` at your line |
| `span`/`static_vector` index out of range, `front`/`back`/`pop_back` on an empty one | the container's message |
| division by zero or overflow with cp's wide integers, `div_sat` by zero | the function's message |
| `*` on a `util::result` holding an error, `error()` on one holding a value (before C++23) | `result: no value` / `result: no error` |
| an integer base outside 2–36 in `io::base` | `integer base outside 2..36` |

Your reports (`CP_CHECK`, `io::dump`, bad input at your `io::read`) go through `io::err`, so they follow it if you
redirect it. The library's own precondition reports (the containers, wide integers, `result`, the base) are written
straight to standard error with the library's file and line.

The debug mode also turns on the address and undefined-behaviour sanitizers (not on MinGW) and libstdc++'s own
assertions (`_GLIBCXX_ASSERTIONS`), which catch out-of-range `std::vector` indexing and similar.

## `debug::check(condition, message)`

The function behind `CP_CHECK`, for a message you compute. Being a function, it evaluates its arguments even with
checks off; the compiler removes conditions without side effects, but prefer `CP_CHECK` in solutions.

## Also in this module

| Name | What it is |
|---|---|
| `debug::source_location` | file, line and function of a point in the program (`std::source_location` from C++20) |
| `debug::unreachable()` | marks a point that is never reached (`std::unreachable` from C++23) |
