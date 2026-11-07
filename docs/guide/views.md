# views: ranges

`#include "cp/views/views.hpp"`. From C++20 `cp::views` is `std::views`, every view included. In C++17 it has
`views::iota` and `views::reverse`.

<!-- example: views.cpp -->
```cpp
// views::iota and views::reverse, also in C++17; binary search over a range of numbers.
#include "cp/io/io.hpp"
#include "cp/views/views.hpp"
#include <algorithm>
#include <vector>
using namespace cp;

int main() {
    long long sum = 0;
    for (int i : views::iota(1, 11)) sum += i;
    std::vector<int> v{1, 2, 3}, backwards;
    for (int x : views::reverse(v)) backwards.push_back(x);

    auto r       = views::iota(0LL, 1'000'000'000LL);                 // the smallest x with x * x >= 10^12
    auto first_x = *std::partition_point(r.begin(), r.end(), [](long long x) { return x * x < 1'000'000'000'000LL; });
    io::writeln(sum, backwards, first_x);     // 55 3 2 1 1000000
}
```

## Binary search over numbers

`std::partition_point` over `views::iota(lo, hi)` finds the first number where a predicate turns false, in
O(log n) steps:

- Use one type for both bounds: `iota(0, n)` with an `int` and a `long long` gives an end that the classic
  algorithms reject. `std::ranges::partition_point` accepts it (C++20).
- `partition_point` finds the first *false*: to find the first `x` where `p(x)` holds, pass `!p`.
- If the predicate holds everywhere the result is the end: check before dereferencing.
- Older standard libraries may step through C++20's `std::views::iota` one element at a time; cp's C++17
  `iota` and libstdc++ 16 do not.
