# Contributing

## Layout

```
include/cp/config.hpp            platform detection, CP_CHECKS
include/cp/<module>/<module>.hpp one module's umbrella; the module's headers next to it
include/cp/detail/               code several modules share (namespace cp::detail)
include/cp/all.hpp               every module except aliases
tests/test_<module>.cpp          tests
bench/                           benchmarks
examples/                        the documentation's examples, compiled with the tests
docs/guide/                      this guide (Markdown; SUMMARY.md is the table of contents)
tools/xmake/                     the xmake tasks: matrix, bench, iobench, bundle, docs
```

## Rules

- **Names:** every public name lives in a module namespace (`cp::num`, `cp::io`, ...) and is short and clear. It
  may equal a std name only when it *is* that std entity (`using std::span;`), so `using namespace std;` next to
  any module never becomes ambiguous.
- **Standards:** use the newest standard facility the language level has, behind its `__cpp_lib_*` macro, and keep
  cp's own version for older levels in the `#else`.
- **No duplicates:** search `include/cp` before adding a helper; shared code lives once in `cp::detail` or
  `cp::traits`.
- **Checks:** library preconditions use `detail::precondition(condition, "message")`, which reports to standard
  error, so every module can use it; `CP_CHECK`/`io::dump` are for contestants and report through `io::err`.
- **Each header compiles on its own** and includes what it uses.
- **Comments:** one `///` line per entity for Doxygen, a short `@par Caveats` list in the file comment for
  behaviour that differs by compiler or standard. No essays.
- **Style:** `.clang-format` and `.clang-tidy` in the repository; `snake_case`, `m_` members, `auto f() -> T` when
  the return type isn't obvious.

## Before sending a change

```sh
xmake matrix          # every compiler and language level
xmake docs --check    # the guide's examples match examples/
```

A new module comes with its tests, an example in `examples/`, a guide page in `docs/guide/` (listed in
`SUMMARY.md`, the `README.md` and the `Doxyfile`'s `INPUT`), and benchmarks if speed matters.
