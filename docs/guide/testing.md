# Testing and benchmarks

Catch2 3.16.1 and Google Benchmark 1.9.5 are pinned in `xmake.lua`, so every machine tests with the same versions.
xmake uses an installed copy when it is exactly that version; otherwise it asks whether to download (checksum-verified)
and build it, the first time you enable tests or benchmarks. Change the version in `xmake.lua` to move to another.

Google Benchmark is pinned one release back: 1.9.5 is the newest xmake's package repository has. The speed table
in [num](num.md#modular-integers) was measured with 1.10.0 (Arch's package) before the pin; the library only times
the code, so those numbers stay comparable.

| Command | What it does |
|---|---|
| `xmake f --tests=y && xmake test` | builds and runs the tests (Catch2), the header checks and the documentation's examples |
| `xmake matrix` | the tests in debug mode for GCC, Clang and MinGW at C++17, 20, 23 and 26, each in its own build directory |
| `xmake matrix --toolchains=gcc --stds=17,23` | a subset |
| `xmake bench [--filter=regex]` | the Google Benchmark programs in `bench/`, release build |
| `xmake iobench [--count=N] [--runs=R]` | reads and writes N numbers with cp, `scanf`/`printf` and `cin`/`cout`, from files and pipes (POSIX shells only) |
| `xmake docs` | the documentation site in `build/docs/html` |
| `xmake docs --check` | checks that the guide's code blocks match `examples/` |

The MinGW cells build Windows programs; they run when `wine` is installed and are only built otherwise.

## Tests

`tests/test_<module>.cpp` holds one module's tests. They are small and target behaviour that is easy to get wrong:
boundaries, overflow, end of input, reports at the caller's line. Tests that need the standard streams run their
code in a child process with pipes for stdin, stdout and stderr (`tests/fd_helpers.hpp`), on Linux and on Windows.

Every header also compiles on its own (the `headers` target), and every file in `examples/` compiles with the
strict warning set.
