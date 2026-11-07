# Getting started

## What you need

| Tool | Needed for |
|---|---|
| a 64-bit C++17 compiler: GCC, Clang or MinGW-w64 GCC (not MSVC) | everything |
| [xmake](https://xmake.io) | building, tests, `xmake bundle` |
| `clang++` | `xmake bundle`'s size reduction (step 4), for every profile; without it bundles are bigger and a warning says so |
| MinGW-w64 GCC | bundles for Windows judges (Codeforces) |
| `wine` (Linux, macOS) | running Windows test programs and `xmake bundle --profile=mingw --sample` |
| Catch2 3, Google Benchmark | tests and benchmarks: xmake offers to download and build them if your system has none, and asks first |

cp is tested on Linux with GCC 16, Clang 23 and MinGW-w64 GCC 16 at C++17, 20, 23 and 26, on x86-64; the Windows
programs it builds run under wine. Developing on Windows or macOS has not been tried. Other targets than x86-64
and AArch64 are rejected at compile time.

## Installing

Install xmake as [xmake.io](https://xmake.io/#/guide/installation) describes, then the compilers.

**Linux.** Package names differ between distributions:

| Distribution | Compilers and clang | MinGW-w64 (Codeforces bundles) | wine |
|---|---|---|---|
| Debian, Ubuntu | `g++ clang` | `g++-mingw-w64-x86-64` | `wine` |
| Fedora | `gcc-c++ clang` | `mingw64-gcc-c++` | `wine` |
| Arch | `gcc clang` | `mingw-w64-gcc` | `wine` |

**Windows.** Install [MSYS2](https://www.msys2.org) and, in its UCRT64 shell,
`pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-clang mingw-w64-ucrt-x86_64-xmake`. That gives MinGW
GCC (the `mingw` profile uses plain `g++` on Windows), `clang++` for bundling, and xmake. A Windows compiler
cannot produce bundles for Linux judges (`xmake bundle` refuses, see [Submitting](bundling.md)): bundle for those
in [WSL](https://learn.microsoft.com/windows/wsl/) with the Linux instructions above.

**macOS.** `xcode-select --install` gives Clang; Homebrew has `xmake`, `mingw-w64` and `wine`. Bundles for Linux
judges need a Linux machine, container or WSL-like VM, for the same reason.

## The template

```sh
git clone https://github.com/RougherO/CP.git && cd CP
```

`main.cpp` is the solution template: write your solution there.

```cpp
#include <bits/stdc++.h>
#include "cp/aliases/aliases.hpp"
#include "cp/io/io.hpp"
using namespace std;
using namespace cp;
using namespace cp::aliases;

void solve(int t [[maybe_unused]]) { }

int main() {
    for (auto t = *io::read<int>(); t--;) solve(t);
}
```

Include a module by its umbrella header, `cp/<module>/<module>.hpp`, or everything with `cp/all.hpp`. Every name
lives in a module namespace: after `using namespace cp;` you write `io::read(x)`, `bits::popcnt(x)`,
`num::u128`. You can also open a module: `using namespace cp::io;`.

## Building and running

```sh
xmake                          # build main.cpp
xmake run main < input.txt     # run it
xmake f -m release && xmake    # an optimised build without checks
xmake f -m debug               # back to the default
xmake f --cxxstd=17            # another language level: 17, 20, 23 (default) or 26
xmake f --toolchain=clang      # another compiler
```

| Mode | Checks (`CP_CHECKS`) | Sanitizers | Use it for |
|---|---|---|---|
| `debug` (default) | on | address and undefined behaviour (not on MinGW), libstdc++ assertions | local runs |
| `release` | off | none | timing |

With checks on, bad input, failed `CP_CHECK`s and library preconditions stop the program with your file and line;
see [Checks and debugging](debugging.md).

## Submitting

```sh
xmake bundle                   # Linux judges (AtCoder and most others)
xmake bundle --profile=mingw   # Windows judges (Codeforces)
```

Both write `build/submit.cpp`, a single file with the cp code your solution uses; submit it. See
[Submitting](bundling.md).

## Without xmake

cp is header-only: add `include/` to the include path.

```sh
g++ -std=c++20 -O2 -I path/to/CP/include main.cpp -o main
g++ -std=c++20 -g -DCP_CHECKS=1 -I path/to/CP/include main.cpp -o main   # with checks
```

`CP_CHECKS` is 0 unless you define it; `xmake`'s debug mode defines it as 1.
