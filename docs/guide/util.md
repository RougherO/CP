# util: result

`#include "cp/util/util.hpp"`. `util::result<T, E>` holds a value of `T` or an error `E`, like Rust's
`Result<T, E>`; `util::success(x)` and `util::failure(e)` make one, and a plain value converts in too. Every
`io::read` returns one.

<!-- example: result.cpp -->
```cpp
// util::result: a value or an error, like Rust's Result; success and failure build one.
#include "cp/io/io.hpp"
#include "cp/util/util.hpp"
#include <string>
using namespace cp;

auto safe_div(long long a, long long b) -> util::result<long long, std::string> {
    if (b == 0) return util::failure(std::string("division by zero"));
    return util::success(a / b);              // or just `return a / b;`
}

int main() {
    auto q = safe_div(7, 2);
    auto z = safe_div(1, 0);
    if (q) io::writeln(*q);                   // 3
    if (!z) io::writeln(z.error());           // division by zero
    io::writeln(z.value_or(-1));              // -1
}
```

| Member | Result |
|---|---|
| `has_value()`, `explicit operator bool` | true when it holds a value |
| `*r`, `r->`, `value()` | the value (only when there is one) |
| `error()` | the error (only when there is none) |
| `value_or(x)` | the value, or `x` |
| `emplace(args...)`, assignment from a value, a `success` or a `failure` | change the contents |
| `swap`, `==`, `!=`, `std::hash` | as for `std::optional` |

`result<void, E>` is success or an error: `return util::success();`.

## Across standards

From C++23 `result` is a `std::expected<T, E>` and `failure` a `std::unexpected<E>`, so every member of
`std::expected` is there, including `and_then`, `transform`, `or_else` and `transform_error` (they return
`std::expected`). Before C++23 `result` is cp's own class with `std::optional`'s interface, without those.

`*r` on an error is a bug. Before C++23 with checks on, the program ends with `result: no value` and the line; from C++23,
`value()` throws `std::bad_expected_access`, as `std::expected` does.
