# Reading and writing

`#include "cp/io/io.hpp"`. Three streams: `io::in` (standard input), `io::out` (standard output, buffered) and
`io::err` (standard error, unbuffered). The free functions `io::read`, `io::readln`, `io::getline`, `io::write` and
`io::writeln` use `io::in` and `io::out`; other streams have the same functions as members (`r.read(x)`).

- `read` skips all whitespace, newlines included: lines don't matter to it.
- `readln(values...)` reads like `read`, then drops the rest of the line with its `\n` (`readln()` just skips to the
  next line). After `readln(n)`, `getline(s)` gets the next line, not the empty rest of `n`'s.
- `getline(s)` reads the raw line, spaces kept.

<!-- example: io_read.cpp -->
```cpp
// Every way to read and write with cp::io.
#include "cp/io/io.hpp"
#include <string>
#include <tuple>
#include <utility>
#include <vector>
using namespace cp;

int main() {
    int n = 0;
    std::string name;
    io::read(n, name);  // into variables, like std::cin >> n >> name

    std::vector<long long> a(n);
    io::read(a);  // a range reads as many elements as it holds

    auto k      = *io::read<int>();       // declare and read
    auto [h, w] = *io::read<int, int>();  // several values, as a tuple

    std::vector<std::pair<int, int>> edges(k);
    io::read(edges);  // pairs, tuples and plain structs read field by field

    std::string line;
    io::getline(line);                        // the rest of the current line
    io::getline(line);                        // the next line
    io::readln(n);                            // n, then the rest of its line is dropped
    auto hex = *io::read<int>(io::base(16));  // with a formatter (see io_options.cpp)

    long long x = 0, sum = 0;
    while (io::read(x)) sum += x;  // until end of input

    io::write(n, name);  // separated by a space, no newline
    io::writeln();       // just a newline
    io::writeln(a);      // a range: elements separated by spaces
    io::writeln(h * w, edges.size(), sum, line, hex);
    io::writeln(std::vector<std::vector<int>>{{1, 2}, {3, 4}});  // nested ranges: one row per line
    io::writeln(edges);                                          // a range of pairs: one pair per line
}
```

## What can be read and written

| Type | Read | Written |
|---|---|---|
| integers, including `i128`/`u128` and wider | a number in base 10 (or `io::base`); a sign for signed types | digits |
| `float`, `double`, `long double` | any form `std::from_chars` accepts | fixed, 9 decimals by default |
| `char` | one character, after skipping whitespace | the character |
| `bool` | a number, non-zero is `true` | `1`/`0` (or `true`/`false`) |
| `std::string` | one whitespace-separated word | the text |
| `std::string_view`, string literals | — | the text |
| ranges (`std::vector`, `std::array`, `std::map`, ...) | as many elements as the range holds (not maps or sets: their keys are `const`) | numbers separated by spaces; ranges of ranges, pairs or structs one per line |
| `std::pair`, `std::tuple`, plain structs | each field in order | fields separated by spaces |
| your own types | `cp_read` | `cp_write` |

`signed char`/`unsigned char` (so `std::int8_t`, `std::uint8_t`) are numbers, not characters. Nothing `const` can be read, at any depth: read a map's pairs, then insert them:
`for (auto n = *io::read<usize>(); n--;) { auto [k, v] = *io::read<i64, i64>(); m[k] = v; }`.

## Results and errors

Every read returns an `io::read_result`: true when it succeeded, otherwise its `error()` says why.

| `io::read_error` | Meaning |
|---|---|
| `end_of_input` | no token was left |
| `invalid` | the token is not a value of that type (`x` for an integer) |
| `out_of_range` | the number does not fit the type (`300` for a `std::uint8_t`) |

`io::read<T>()` returns the value or the error: `*io::read<int>()` takes the value, `value_or(d)` gives a
fallback. A failed read stops the reads after it, like `std::cin`, until `io::in.clear()`. A bad token stays unread;
`io::in.token()` shows it.

With checks on (`CP_CHECKS`), `invalid` and `out_of_range` stop the program with your line, the input line and the
token:

```
main.cpp:12: int main(): cp::io::read: input line 3: invalid token "x"
```

`end_of_input` is never reported: it is how `while (io::read(x))` loops end.

<!-- example: io_errors.cpp -->
```cpp
// A read returns a result: success, or why it failed.
#include "cp/io/io.hpp"
using namespace cp;

int main() {
    int x = 0;
    if (auto r = io::read(x); !r) {
        switch (r.error()) {
            case io::read_error::end_of_input: io::writeln("no input"); break;
            case io::read_error::invalid:      io::writeln("not a number:", io::in.token()); break;
            case io::read_error::out_of_range: io::writeln("too big:", io::in.token()); break;
        }
        return 0;
    }
    auto y = io::read<long long>();           // a value or an error
    io::writeln(x + y.value_or(0));
}
```

## Formatters

Streams keep no settings: every call starts from the defaults below, and a formatter passed in the call changes
them for that call only.

- **As an argument**, a formatter governs the arguments after it, up to the next formatter, which replaces it:
  `io::writeln(io::precision(3), x, y, io::defaults(), z)`. `io::defaults()` gives back the defaults.
- **Called on values**, it decorates only those: `io::writeln("ans:", io::range_sep(",")(v), k)`. The result holds
  references, so use it in the same statement.
- **`f | g`** applies both, the right one winning on the same setting. Keep presets as
  constants: `constexpr auto py = io::range_sep(", ") | io::range_bracket("[", "]");`. A formatter holds its strings
  as `std::string_view`, so build kept ones from literals or strings that outlive them, never from a temporary
  `std::string`.
- **Reading** takes them the same way: `io::read(io::base(16), h)`, `auto h = *io::read<int>(io::base(16));`.

| Formatter | Reading | Writing | Default |
|---|---|---|---|
| `base(b)` | yes | yes | 10 (2–36) |
| `float_format(f)` | yes | yes | `general` reading, `fixed` writing (`std::chars_format`) |
| `precision(p)` | | yes | 9 digits after the point; negative: shortest exact |
| `width(w)`, `fill(c)`, `show_plus()` | | yes | 0, a space, no `+` (integers) |
| `show_bool_words()` | | yes | `1`/`0` |
| `skip_ws(b)` | yes | | `char` skips whitespace |
| `arg_sep(s)` | | yes | `" "` between arguments |
| `range_sep(s1, s2, ...)` | | yes | `" "`, `"\n"`: one separator per nesting level, innermost first; deeper levels reuse the last |
| `field_sep(s)` | | yes | `" "` between the fields of pairs, tuples and structs |
| `range_bracket(open, close)`, `field_bracket(open, close)` | | yes | none |
| `show_quotes()` | | yes | strings in `"…"`, chars in `'…'`, escaped |
| `show_field_names()` | | yes | structs as `name: value` (from C++20; plain fields before) |
| `wrap(w)` | | yes | off; a range or struct wider than `w` goes one element per line, indented |

Nesting levels: a number, string or char is level 0, a range is one above its elements, and a pair, tuple or struct
one above its highest field. So `vector<int>` uses the first `range_sep`, while `vector<vector<int>>`, `map` and
`vector<pair>` use the second: one row or pair per line.

<!-- example: io_options.cpp -->
```cpp
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
```

## Interactive problems

`io::in` is tied to `io::out`: before the program waits for more input, everything written so far is flushed. You
never call `flush` yourself.

<!-- example: io_interactive.cpp -->
```cpp
// An interactive problem: guess a number in [1, 10^6]; the judge answers 1 if the secret is at most the query.
#include "cp/io/io.hpp"
using namespace cp;

int main() {
    int lo = 1, hi = 1'000'000;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        io::writeln('?', mid);                // no flush needed: reading flushes io::out first
        if (*io::read<int>() == 1) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
    io::writeln('!', lo);
}
```

The tie costs nothing measurable: it flushes only when the reader is about to wait on a pipe, never for a file.

## Your own types

A plain struct (public fields, no constructors, no base classes) is read and written field by field with no code,
up to 32 fields (any number with C++26 reflection). For other types define `cp_read` and `cp_write` next to the
type; cp finds them by argument-dependent lookup.

<!-- example: io_custom.cpp -->
```cpp
// Your own types: plain structs need nothing; others define cp_read / cp_write next to the type.
#include "cp/io/io.hpp"
#include <vector>
using namespace cp;

struct edge {                                 // a plain struct: its fields are read and written in order
    int u, v;
    long long w;
};

class fraction {                              // not a plain struct: say how to read and write it
public:
    long long num = 0, den = 1;
};
void cp_read(io::reader& r, fraction& f) { r.read(f.num, f.den); }
void cp_write(io::writer& w, fraction const& f) {
    w.write(f.num);
    w.put('/');
    w.write(f.den);
}

int main() {
    std::vector<edge> edges(*io::read<int>());
    io::read(edges);
    fraction f;
    io::read(f);
    for (auto const& e : edges) io::writeln(e);  // u v w
    io::writeln(f);                              // num/den
}
```

## Speed

Reading and writing 10⁷ integers (`xmake iobench`, GCC -O2, milliseconds, less is better):

| | read file | read pipe | write file | write pipe |
|---|---|---|---|---|
| cp | 142 | 155 | 166 | 143 |
| `cin`/`cout`, untied | 490 | 500 | 592 | 379 |
| `scanf`/`printf` | 636 | 677 | 647 | 489 |

Input from a file is mapped into memory; from a pipe it is read in 64 KiB blocks. Output is buffered in 64 KiB.

## Things to know

- Don't mix `io::out` with `printf` or `std::cout`: each has its own buffer, so the output comes out in the wrong
  order.
- `io::out` is flushed at exit. After `std::quick_exit` or a crash, output still in the buffer is lost; call
  `io::out.flush()` first if that matters.
- `getline` drops a trailing `\r`, so Windows line endings read like Unix ones.
- A token longer than 64 KiB is cut at that size (strings are not).
- After `using namespace cp;` at global scope, a POSIX `write(fd, buf, n)` call means `cp::io::write`; write
  `::write`.
