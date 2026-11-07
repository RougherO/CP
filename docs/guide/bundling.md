# Submitting

Judges take one file. `xmake bundle` turns your solution and the cp headers it includes into `build/submit.cpp`.

```sh
xmake bundle [--profile=gcc|clang|mingw] [--std=c++20] [--checks=0|1] [--sample=in.txt]
             [--input=main.cpp] [--output=build/submit.cpp]
```

## Which profile

| Judge | Profile | Needs |
|---|---|---|
| Codeforces (Windows) | `mingw` | MinGW-w64's `x86_64-w64-mingw32-g++` (plain `g++` on Windows); to check `--sample` on Linux or macOS, `wine` |
| AtCoder, CodeChef, most others (Linux, GCC) | `gcc` (default) | `g++` targeting Linux: bundle on Linux or WSL |
| Linux judges with Clang | `clang` | `clang++` targeting Linux: bundle on Linux or WSL |

Every profile also uses `clang++` for step 4 below; see [Installing](getting-started.md#installing) for each
system.

Pass the judge's language level with `--std` (`c++17`, `c++20`, `c++23`, `c++26`); the default is `c++20`. A bundle
is made for one compiler, operating system and language level: a Linux bundle does not compile on Windows.

## What it does

1. **Inlines** every cp header your solution includes, once each. Your code after the last cp include is kept
   exactly as written.
2. **Resolves every `#if`** with the judge's settings: its standard library's features for `--std`, its operating
   system, and `CP_CHECKS`. Only the code that judge compiles remains.
3. **Strips** comments, blank lines and indentation.
4. **Drops what you don't use:** cp declarations your solution never reaches, then code no template instance
   reaches: function bodies nothing calls, `if constexpr` branches nothing takes, class templates nothing uses.
   Repeated until nothing changes. Needs `clang++` for the analysis, even for the `gcc` and `mingw` profiles;
   without it this step is skipped with a warning.
5. **Verifies:** compiles the result with the profile's compiler. With `--sample in.txt`, also runs your unbundled
   solution and the bundle on that input and requires the same output and exit code.
6. **Checks the size:** fails above 65,535 bytes, lists the largest declarations kept and writes nothing.

Including `cp/all.hpp` is fine: step 4 removes what you don't use. The examples bundle to 12–41 KB; an io-only
solution to about 28 KB.

## Checks in a submission

`--checks=1` keeps `CP_CHECK`, `io::dump` and the library's checks in the submission: a wrong answer caused by bad
input or a broken invariant then shows up as a runtime error with your line in the judge's error output, where the
judge shows it. The default, `--checks=0`, removes them.

## When something goes wrong

- *"profile mingw needs x86_64-w64-mingw32-g++ on PATH"*: install MinGW-w64.
- *"... does not target the gcc profile's system"*: that compiler builds for another operating system (for example
  `g++` on macOS); bundle on Linux or with a cross compiler.
- *"the shaken and pruned bundle does not compile ... trying it shaken"*: step 4 removed too much. The next, less
  reduced bundle is tried and written if it passes; the failed one is kept as `build/failed-<stage>.cpp` with the
  compiler's errors shown. Please report it.
- *"over the 65535-byte limit"*: the message lists the largest declarations kept; drop the headers you don't need,
  or use a higher `--std` (newer standards need less of cp's own code).
- Your compiler is newer than the judge's: a library feature it has and the judge's lacks would pick the wrong
  code. Submit once with each new `--std` to confirm it compiles there.
