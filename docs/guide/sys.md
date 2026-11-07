# sys: the machine

`#include "cp/sys/sys.hpp"`.

<!-- example: sys.cpp -->
```cpp
// What the CPU running the program supports.
#include "cp/io/io.hpp"
#include "cp/sys/sys.hpp"
using namespace cp;

int main() {
    io::writeln(sys::cpu_supports(sys::cpu_feature::avx2), sys::cpu_supports(sys::cpu_feature::popcnt));
}
```

| Name | What it is |
|---|---|
| `sys::cpu_supports(f)` | whether the CPU running the program has feature `f` |
| `sys::cpu_feature` | `popcnt`, `sse42`, `bmi2`, `avx2`, `avx512f` (x86), `neon` (ARM) |
| `sys::prefetch(p)` | asks the CPU to load `*p` into cache |

`cpu_supports` answers for the machine the program runs on; `CP_HAS_AVX2` and the other `CP_HAS_*` macros answer
for the flags it was compiled with. To use AVX2 only where it exists, compile a function with `CP_TARGET("avx2")`
and call it after `sys::cpu_supports(sys::cpu_feature::avx2)`.
