# ds: containers

`#include "cp/ds/ds.hpp"`.

<!-- example: ds.cpp -->
```cpp
// span, static_vector and ssize.
#include "cp/ds/ds.hpp"
#include "cp/io/io.hpp"
#include <vector>
using namespace cp;

int main() {
    std::vector<int> v{5, 1, 4};
    ds::span<int> s(v);                       // a view of v's elements
    io::writeln(s.size(), s.first(2), ds::ssize(v) - 4);

    ds::static_vector<int, 4> small{3, 1, 2}; // at most 4 ints, stored inline: no allocation
    small.push_back(7);
    erase(small, 1);                          // found by argument-dependent lookup
    io::writeln(small, small.size(), small.capacity());
}
```

| Name | What it is |
|---|---|
| `ds::span<T>` | a view of contiguous elements: `std::span` from C++20 (static extents, `as_bytes` too); before, cp's own with a dynamic extent only |
| `ds::static_vector<T, N>` | a vector of at most `N` elements stored inline, no allocation (`std::inplace_vector` from C++26) |
| `ds::ssize(c)` | a container's size as a signed integer (`std::ssize` from C++20) |

## `static_vector`

The interface of `std::vector` without capacity growth: `push_back` past `N` throws `std::bad_alloc`, as
`std::inplace_vector` does; `try_push_back` and `try_emplace_back` return an empty result instead. `erase(v, x)` and `erase_if(v, pred)` remove matching elements;
call them unqualified.

With checks on, `operator[]`, `front`, `back` and `pop_back` check their preconditions.

Before C++26 the `try_*` functions return a small reference wrapper: use only `if (r)`, `*r`, `r->`,
`has_value()` and `value()` on it.
