# cp

A header-only C++ library for competitive programming. It works from C++17 to C++26 with the same names, and the
newest standard facility does the work whenever the compiler has it.

- **Fast I/O:** reads and writes 10⁷ integers 3–4× faster than `scanf`/`printf` or untied `cin`/`cout`, from files and
  pipes, and handles interactive problems without manual flushing.
- **Errors you can see:** with checks on, bad input, failed `CP_CHECK`s and library preconditions stop the program
  at your line. Submissions compile them away.
- **Modules:** 128-bit and wider integers, saturating arithmetic, bit operations, `span`, `static_vector`, ranges
  backports, a `result` type, type traits, CPU feature detection.
- **One-file submissions:** `xmake bundle` inlines what your solution uses, keeps only the code your judge's
  compiler needs, and checks that the result compiles, fits in 64 KB and prints the same output.

<!-- example: hello.cpp -->
```cpp
// Sum of each test case's numbers: the shape of most solutions.
#include "cp/io/io.hpp"
#include <vector>
using namespace cp;

int main() {
    for (auto t = *io::read<int>(); t--;) {
        auto n = *io::read<int>();
        std::vector<long long> a(n);
        io::read(a);
        long long sum = 0;
        for (auto x : a) sum += x;
        io::writeln(sum);
    }
}
```

## Quick start

You need a C++17 compiler (GCC, Clang or MinGW-w64), [xmake](https://xmake.io), and `clang++` for the smallest
submissions; [Installing](docs/guide/getting-started.md#installing) lists the packages for Linux, Windows and macOS.

```sh
git clone https://github.com/RougherO/CP.git && cd CP
xmake                          # builds main.cpp (debug: checks and sanitizers on)
xmake run main < input.txt     # run your solution
xmake bundle                   # writes build/submit.cpp: submit that file
```

Edit `main.cpp`, the solution template. For Codeforces (Windows judges) bundle with `xmake bundle --profile=mingw`;
see [Submitting](docs/guide/bundling.md).

Without xmake, add `include/` to your include path: `g++ -std=c++20 -O2 -I path/to/CP/include main.cpp`.

## Documentation

The guide, in reading order:

1. [Getting started](docs/guide/getting-started.md): install, the template, build modes.
2. [Reading and writing](docs/guide/io.md): every form of `io::read` and `io::write`, formatters, interactive
   problems, your own types.
3. [Checks and debugging](docs/guide/debugging.md): `CP_CHECK`, `io::dump`, what checked builds catch.
4. Modules: [num](docs/guide/num.md), [bits](docs/guide/bits.md), [ds](docs/guide/ds.md),
   [views](docs/guide/views.md), [util](docs/guide/util.md), [traits](docs/guide/traits.md),
   [sys](docs/guide/sys.md), [aliases](docs/guide/aliases.md).
5. [Language levels](docs/guide/language-levels.md): C++17 to C++26, and `using namespace std;` with cp.
6. [Submitting](docs/guide/bundling.md): `xmake bundle` and judge profiles.
7. [Testing and benchmarks](docs/guide/testing.md).
8. [Contributing](docs/guide/contributing.md).

The same pages and the API reference form the documentation site: run `xmake docs` and open
`build/docs/html/index.html`.

## License

[MIT No Attribution](https://github.com/RougherO/CP/blob/master/LICENSE): use, change and submit cp's code anywhere; bundles need no copyright notice.
