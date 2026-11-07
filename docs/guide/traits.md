# traits: type traits

`#include "cp/traits/traits.hpp"`: traits that work from C++17 on.

<!-- example: traits.cpp -->
```cpp
// Type traits that work from C++17 on.
#include "cp/io/io.hpp"
#include "cp/traits/traits.hpp"
#include <map>
#include <vector>
using namespace cp;

template <class T>
using size_of = decltype(std::declval<T&>().size());

enum class colour : unsigned char { red = 1, green = 2 };

int main() {
    io::writeln(traits::is_detected_v<size_of, std::vector<int>>, traits::is_detected_v<size_of, int>);   // 1 0
    io::writeln(traits::is_specialization_of_v<std::map<int, int>, std::map>);                             // 1
    io::writeln(traits::to_underlying(colour::green));                                                    // 2
}
```

| Name | What it is |
|---|---|
| `traits::is_detected_v<Op, Args...>`, `is_detected`, `detected_t`, `nonesuch` | whether `Op<Args...>` is valid: the detection idiom |
| `traits::is_specialization_of_v<T, Template>` | whether `T` is `Template<...>`: `is_specialization_of_v<std::vector<int>, std::vector>` |
| `traits::remove_cvref_t<T>` | `T` without references and `const` (`std::remove_cvref_t` from C++20) |
| `traits::type_identity_t<T>` | `T`, in a context that blocks deduction (`std::type_identity_t` from C++20) |
| `traits::is_constant_evaluated()` | whether the call runs at compile time (`std::is_constant_evaluated` from C++20) |
| `traits::to_underlying(e)` | an enumeration's value as its underlying type (`std::to_underlying` from C++23) |

`is_specialization_of_v` takes templates with type parameters only.
