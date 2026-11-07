# `cp` — Competitive Programming Library: Design

Date: 2026-11-04
Status: approved 2026-11-04
Replaces: the single-file template in `main.cpp`

Decisions are marked **[owner]** when the owner decided them and **[my call]**
when they were proposed by the designer and not objected to. The owner may
revisit any **[my call]** item.

---

## 1. Intent

A personal competitive-programming library that:

- makes input/output never the bottleneck of a solution;
- uses modern C++ deliberately, as a hardened, user-friendly library, not as
  a collection of contest macros and shorthand;
- feels like the standard library to anyone who knows it;
- compiles as C++17 today while already exposing C++20/23/26 facilities, and
  upgrades to each newer standard by raising one floor macro and deleting the
  old branches;
- is developed as small headers (testable, benchmarkable, profilable) but is
  submitted, and shared, as a single self-contained file.

## 2. Goals, non-goals, anti-goals

**Goals**

1. Fastest practical I/O on Linux and Windows judges, including interactive
   problems.
2. One API surface across C++17 → C++26. Solution code does not change when
   the language level changes.
3. Every public type and algorithm behaves like its closest standard-library
   counterpart (names, signatures, guarantees, customization points).
4. Errors are caught early: compile-time where the language allows, checked
   builds report the *user's* call site, release builds pay nothing.
5. Unit tests (Catch2), benchmarks (Google Benchmark) and `perf` work on the
   library directly.
6. A bundled submission fits under 64 KB (Codeforces source limit).

**Non-goals**

- Supporting compilers older than GCC 11 and Clang 14 **[my call]**, or MSVC at all
  **[owner, 2026-11-07: "drop support for MSVC completely keep windows support"]**.
- 32-bit targets. Every supported target is 64-bit **[owner]**.
- Full `std::ranges` or full `std::format` reimplementation in C++17. Only
  the subset listed here is backported.
- Global-scope convenience: the library lives in `namespace cp`; solutions
  opt in with `using namespace cp;`.

**Anti-goals** (the design has failed if any of these happen)

- User-facing macros for solving (no `#define int long long`, no loop
  macros, no `ll`/`vi`/`pb`) **[owner]**. The one exception is `CP_CHECK`
  (§10): a function cannot leave its arguments unevaluated
  **[owner, 2026-11-06]**; `CP_DBG` was replaced by the function
  `io::dump` **[owner, 2026-11-07]**.
- Shorthand aliases for things that already exist in C++17 (no `vec`, no
  `cp::vector`, no `cp::string`) **[owner]**.
- Too many ways to do the same thing. Each need has one obvious mechanism
  **[owner]**.
- Hidden global formatting state that a call can silently change.

## 3. Constraints

- **Language baseline:** compiles with `-std=c++17`. Must also compile, with
  identical behaviour, at `-std=c++20`, `c++23` and `c++26`.
- **Compilers:** GCC ≥ 11 and Clang ≥ 14, nothing else; `config.hpp` stops
  any other compiler with `#error` **[owner, 2026-11-07]**.
- **64-bit only** **[owner]**, x86-64 or AArch64, with `__int128`
  (`__SIZEOF_INT128__`, else `#error`).
- **Platforms:** Linux (most judges, local dev), Windows through MinGW-w64
  (Codeforces), macOS (local dev only).
- **No `bits/stdc++.h` inside library headers.** Each header includes the
  standard headers it uses. (`bits/stdc++.h` does not exist on
  macOS Clang; relying on it is what broke `assert` under GCC 16.)
- **ODR-safe:** every non-template function and every namespace-scope
  object in a header is `inline`, so multiple translation units (tests,
  benchmarks) link.
- **Judge safety:** never compile the whole program with `-march` or
  `#pragma GCC target`; a judge CPU lacking the instruction set crashes the
  program with an illegal-instruction signal.

---

## 4. Repository layout

```
CP/
├── xmake.lua
├── include/cp/
│   ├── config.hpp            configuration macros (see §5); includes <version>
│   ├── all.hpp               every module umbrella (not aliases)
│   ├── detail/               internals shared by all modules (double_int, precondition)
│   ├── num/                  cp::num   int128, wide, arith (sat, midpoint), cmp (orderings, fits); num.hpp
│   ├── bits/bits.hpp         cp::bits  popcnt clz ctz clo cto width floor2 ceil2 is_pow2 rotl rotr bswap cast endian
│   ├── ds/                   cp::ds    span, static_vector, ssize; ds.hpp
│   ├── views/views.hpp       cp::views iota, reverse (std::views from C++20)
│   ├── traits/traits.hpp     cp::traits detection idiom, remove_cvref_t, type_identity_t, ...
│   ├── debug/                cp::debug source_location, unreachable (Plan 3: check, dbg); debug.hpp
│   ├── sys/                  cp::sys   cpu (cpu_supports, prefetch); input_source/output_sink (internal); sys.hpp
│   ├── io/                   cp::io    options, codec, aggregate, parse, format, reader, writer; io.hpp
│   ├── aliases/aliases.hpp   cp::aliases (opt-in, §11)
│   └── math/ graph/ str/ …   later modules (see §12)
├── main.cpp                  solution template (thin)
├── tests/                    Catch2, one test_<module>.cpp per header
├── bench/                    Google Benchmark; bench/io: whole-program I/O race (`xmake iobench`)
├── stress/                   gen.cpp, brute.cpp, stress script
├── tools/
│   ├── xmake/                xmake tasks: matrix, bench, iobench; later bundle, migrate, prune (§14)
│   └── profiles/             compiler, flags and macro values per judge target
├── examples/                 compiled doc examples (§15a)
├── docs/guide/               user guide, GitBook layout (§15a)
├── README.md
└── dist/cp.hpp               generated single header (committed)
```

Header rules: `#pragma once`; self-contained (each header compiles alone —
enforced by a test target that compiles every header in isolation); no
`using namespace` at namespace scope; no `main`.

The 500 KB HTML topic list currently tracked in git is removed from the repo
**[my call]**.

---

## 5. Configuration and platform layer (`config.hpp`)

`config.hpp` defines the configuration macros; the only other macro is
`CP_CHECK` (`debug/check.hpp`), §10. All macros
are prefixed `CP_`. They exist only where the language offers no alternative:
version detection, compiler/OS/architecture detection, attributes, and
arguments that must not be evaluated.

### 5.1 Language level

`config.hpp` requires C++17 (`static_assert(__cplusplus >= 201703L)`). There
are no `CP_CXXnn` macros: every feature is detected by its feature-test macro
(§6.1) **[2026-11-07: unused config macros removed, added back when needed]**.

### 5.2 Detection

| Macro | Notes |
|---|---|
| `CP_OS_WINDOWS`, `CP_OS_POSIX` | `CP_OS_POSIX` is set on Linux and macOS. |
| `CP_ARCH_X64`, `CP_ARCH_ARM64` | Any other architecture, and any 32-bit target, is a hard `#error`. |
| `CP_HAS_SSE42`, `CP_HAS_AVX2`, `CP_HAS_NEON` | From compiler-predefined macros (compile-time availability only). |
| `CP_HAS_BUILTIN_COLUMN` | `__builtin_COLUMN` exists (source_location's column). |
| `CP_CHECKS` | 0 by default; xmake's debug mode defines it as 1. Submissions run unchecked unless `xmake bundle --checks=1`, the one submission switch **[owner, 2026-11-06: no `LOCAL` indirection, one place]**. |

### 5.3 Attributes

| Macro | Expansion |
|---|---|
| `CP_NOINLINE` | `__attribute__((noinline))` |
| `CP_LIKELY(x)` / `CP_UNLIKELY(x)` | `__builtin_expect(!!(x), 1/0)` |
| `CP_TARGET(isa)` | `__attribute__((target(isa)))` |

### 5.4 Intrinsics, exposed as functions

Intrinsics are wrapped in `constexpr` functions, never exposed as macros:
`__builtin_clzll`/`ctzll`/`popcountll` (bits), `__builtin_bit_cast`,
`__builtin_FILE/LINE/COLUMN/FUNCTION` (source_location),
`__builtin_unreachable`, `unsigned __int128` (wide products),
`__builtin_add_overflow`/`mul_overflow`, `__builtin_prefetch`,
`__builtin_cpu_supports`.

### 5.5 SIMD

- **Opt-in only.** Never enabled for the whole program (judge safety).
- SIMD code lives in individual functions marked `CP_TARGET("avx2")` (or
  similar) and is selected at runtime via `cp::sys::cpu_supports`, with a scalar
  fallback always present.
- `intrin/simd.hpp` offers thin helpers for user algorithms (bulk min/max,
  bitset-style word operations). The I/O core does **not** use explicit SIMD;
  it uses SWAR (§8.4), which is portable and needs no runtime dispatch.
- Once `CP_MIN_CXX` reaches C++26 and `__cpp_lib_simd` is available,
  `simd.hpp` is reimplemented on `std::simd` (internal replacement, §6.2).

---

## 6. Versioning across C++17 → 20 → 23 → 26

### 6.1 Detection rule

- **Library features** are detected with `__cpp_lib_*` macros from
  `<version>` (`config.hpp` includes `<version>` when
  `__has_include(<version>)`, which GCC 9+/Clang 7+ provide in C++17
  mode).
- **Language features** are detected with `__cpp_*` macros, e.g.
  `__cpp_concepts`, `__cpp_consteval`, `__cpp_nontype_template_args`,
  `__cpp_explicit_this_parameter`, `__cpp_impl_reflection`,
  `__cpp_contracts`.
- `CP_CXXnn` is never used to decide whether one feature exists.

### 6.2 Three ways a newer-standard feature enters

| Kind | Rule | Examples |
|---|---|---|
| **Alias or backport** (library) | `cp::<module>::x` is a using-declaration of `std::x` when the feature macro is defined, otherwise a cp implementation with the same interface. | C++20: `bit_cast`, `<bit>`, `span`, `source_location`, `cmp_*`, `in_range`, `midpoint`, `ssize`, `remove_cvref_t`, `type_identity`, `is_constant_evaluated`. C++23: `unreachable`, `to_underlying`, `byteswap`. C++26: `inplace_vector`, saturating arithmetic. |
| **Additive** (language) | Extra overloads/specializations compiled only when available; the C++17 path keeps working. | C++20: `operator<=>`, `std::formatter<cp::T>` specializations, `std::ranges` concept constraints, `consteval` format-string checking. C++23: multi-index `operator[]` on grid types. C++26: reflection-based aggregate codec, contracts behind `CP_CHECK`. |
| **Internal replacement** | Internals switch when `CP_MIN_CXX` rises; public API unchanged. | `if consteval` replaces `is_constant_evaluated()`; deducing `this` removes const/non-const duplicates; `std::simd` replaces `intrin/simd.hpp` internals; `[[assume]]`/`[[likely]]` replace attribute macros. |

### 6.3 Modules and names **[owner, 2026-11-06]** (supersedes the std-mirror layout)

- Every public name lives in a **module namespace**; `cp::` itself holds only
  the modules. Solutions write `using namespace cp;` and then `io::read(x)`,
  `bits::popcnt(x)`, or open a module (`using namespace cp::io;`).
- Modules: `cp::num` (i128/u128, `widen_t`, `wide`, `add/sub/mul_wide`,
  `add/sub/mul/div_sat`, `cast_sat`, `midpoint`, `cmp`, `fits`, the ordering
  types, Plan 5: modint), `cp::bits` (`popcnt clz ctz clo cto width floor2 ceil2 is_pow2 rotl
  rotr bswap cast endian`), `cp::ds` (`span`, `static_vector`, `ssize`),
  `cp::views` (`iota`, `reverse`), `cp::traits` (detection idiom,
  `remove_cvref_t`, `type_identity_t`, `is_constant_evaluated`,
  `to_underlying`), `cp::debug` (`source_location`, `unreachable`; Plan 3:
  `check`, `dbg`), `cp::sys` (`cpu_supports`, `prefetch`), `cp::util`
  (`result<T, E>`, `success`, `failure`: Rust's `Result`, `Ok`, `Err`; from
  C++23 derived from `std::expected`/`std::unexpected`, so the monadic members
  come with them, and before that cp's own class with C++17 `std::optional`'s
  interface; not an alias because libstdc++'s `[[nodiscard]]` on
  `std::expected` would flag every `io::read(x);`), `cp::io`, and the
  opt-in `cp::aliases`. Later: `cp::math`, `cp::ds` (more), `cp::graph`,
  `cp::str`. Internals of every module share `cp::detail`.
- **Names are cp's own**, short and clear, not std's (e.g. `bits::clz`, not
  `countl_zero`). A cp name may equal a std name only when it *is* that std
  entity from the standard that has it (`rotl`, `span`, `midpoint`,
  `strong_ordering`), so `using namespace std;` next to any cp module is never
  ambiguous. Module names avoid std namespaces (`traits`, not `meta`, which is
  C++26 `std::meta`).
- **Implementation:** the std facility does the work when the active standard
  has it and it is no weaker than cp needs (by alias when the name matches, by
  a call inside cp's function otherwise); cp's own code otherwise. Tagged
  `// cp:since c++NN` as before.
- Each module is a directory `cp/<module>/` with an umbrella
  `cp/<module>/<module>.hpp` that includes all of it; `cp/all.hpp` includes
  every umbrella except `aliases`.

### 6.4 Concepts across standards

```cpp
template <class T>
using read_expr = decltype(codec<T>::read(std::declval<reader&>(), std::declval<T&>()));
template <class T> inline constexpr bool readable_v = is_detected_v<read_expr, T>;
```

**No `CP_CONCEPT` macro [owner, 2026-11-04].** A macro that spells `concept`
in C++20 and `inline constexpr bool` in C++17 buys nothing: its body must be
valid C++17, so it can never use a `requires` expression, and C++17 code can't
use it as a constraint. While C++17 is supported, traits are plain
`inline constexpr bool *_v` variables. Real concepts (with `requires`
expressions) are added only once `CP_MIN_CXX` reaches C++20, as an internal
replacement (§6.2).

Overload selection uses `if constexpr` dispatch plus a `static_assert` with a
specific message; `std::enable_if_t<…, int> = 0` is used only where overload
resolution genuinely requires it **[my call]**.

### 6.5 Phase-out tooling

- Every versioned branch is tagged with a comment on its `#if` line:
  `// cp:since c++20` (or 23, 26).
- `xmake prune --min=23` (§14.4) deletes the fallback (`#else`) side of every
  branch tagged at or below the new minimum, keeps the std side, and removes
  the now-empty `#if/#endif`.
- Raising the floor is: set `CP_MIN_CXX`, run `xmake prune`, run the test
  matrix (§13.3).
- These tags are also the source of the alias table that `xmake migrate`
  uses (§14.3).

---

## 7. Standard-library fidelity rules **[owner]**

1. **Counterpart exists in std** → cp's version behaves like it: template
   parameters, member typedefs, `constexpr`/`noexcept`/`[[nodiscard]]`,
   complexity guarantees, exception/precondition behaviour. The name is cp's
   own (§6.3); when std provides the facility and it is no weaker, cp's
   function is that facility or calls it.
   **Exception:** I/O. cp's formatted output is cp's own fast implementation
   and is never an alias of `std::print`.
2. **No counterpart** (`dsu`, `fenwick_tree`, `segment_tree`, `modint`, …) →
   follow std conventions: snake_case; containers provide `value_type`,
   `size_type`, `size()`, `empty()`, `begin()/end()`, `operator[]`
   (unchecked except under `CP_CHECKS`), `at()` (always checked);
   number-like types behave like built-in arithmetic types.
3. **Algorithms** come as an iterator-pair overload and a range overload,
   both in their module namespace **[my call; module rule: owner 2026-11-06]**.
   There is no `cp::ranges` namespace, because `ranges::x` would be ambiguous
   when a solution has both `using namespace std;` and `using namespace cp;`. Under C++20 the range
   overloads are constrained with `std::ranges` concepts so std views are
   accepted.
4. **Every cp type participates in std customization points** where
   meaningful: `std::hash`, `std::tuple_size`/`get`, `std::numeric_limits`
   (number-like types), `operator<<`/`operator>>` for iostreams,
   `std::formatter` (when `__cpp_lib_format`).
5. **Shared conformance tests:** where a std counterpart exists, one typed
   Catch2 suite runs against both (e.g. `cp::ds::static_vector` tests also
   run against `std::inplace_vector` when available).

---

## 8. I/O

All public names below live in `cp::io` (§6.3): `cp::io::read` here means
`cp::io::read`, `cp::io::in` means `cp::io::in`, and so on.

The I/O subsystem is the centerpiece. Requirements: never the bottleneck;
correct on file, pipe and interactive input; identical behaviour on Linux and
Windows.

### 8.1 Input source (`sys/input_source.hpp`)

Chosen once, when `cp::io::in` is first used:

| Situation | Strategy |
|---|---|
| POSIX, `fstat(0)` reports a regular file | `mmap` the whole file read-only. Exception: if the file size is an exact multiple of the page size, reading the padding bytes past the end would raise SIGBUS, so read the whole file into a heap buffer instead. |
| POSIX, pipe or TTY | Chunked `read(2)`. Returns what is available, so it never waits for a full buffer. |
| Windows | `_setmode(_fileno(stdin), _O_BINARY)`, then chunked `_read` (or `ReadFile`). |

**`fread` is never used for input**: it blocks until the buffer is full or
EOF, which deadlocks interactive problems.

### 8.2 Buffer invariant

- The reader guarantees at least **64 readable bytes** after the cursor
  before parsing any token, terminated by a `'\0'` sentinel at end of input.
  When fewer remain, the unread tail is moved to the front and the buffer is
  refilled.
- Numeric parsing therefore performs no end-of-buffer checks inside the
  token loop. Tokens longer than 64 bytes (long strings) take a slow path
  that refills as needed.
- The buffer size is a template parameter of the reader (default 64 KiB) so
  tests can instantiate tiny buffers to force every boundary case.

### 8.3 Output sink (`sys/output_sink.hpp`)

- Writer buffer: 64 KiB. Flushed when fewer than 64 bytes of space remain,
  so writing a single number never checks capacity.
- Sink: `write(1, …)` on POSIX; `_setmode(_O_BINARY)` + `_write`/`WriteFile`
  on Windows. Output newlines are always `\n`.
- Flushed on: explicit `flush()`, destruction, and the tie rule (§8.5).

### 8.4 Parsing and formatting

- Whitespace test: `c <= ' '` (no locale, no signed-char UB). `\r` is
  whitespace.
- Integers: optional sign, then 8 digits at a time via SWAR (8-byte load with
  `memcpy`, digit validation with bit masks, combine with the
  three-multiplication method), scalar tail. Overflow is always
  detected (`detail::append_digit`: compare against `max / base` and
  `max % base`) and fails the read; under `CP_CHECKS` it is reported (§10).
- Floats: `std::from_chars`/`std::to_chars` when `__cpp_lib_to_chars` is
  complete for floating point; otherwise a hand-written parser and
  `snprintf` formatting.
- Integer formatting: two-digit lookup table (`"00"…"99"`).
- `__int128`/`unsigned __int128` supported where `CP_HAS_INT128`.
- **Performance targets** (validated by benchmarks, §13.2, not assumed):
  reading and writing 10⁷ 64-bit integers each must beat
  `scanf/printf` and unsynced `cin/cout`, and must be no slower than the
  current template's reader/writer.

### 8.5 Interactive problems: tie

`cp::io::in` is tied to `cp::io::out`: before the reader performs a refill that may
block (pipe/TTY source), it flushes `cp::io::out`. With `mmap`/slurped input the
tie never triggers. Interactive solutions never need a manual flush.

### 8.6 Streams

```cpp
namespace cp {
inline reader in;    // stdin
inline writer out;   // stdout
inline writer err;   // stderr, unbuffered (writes through immediately)
}
```

`cp::io::err` writes through on every call so diagnostics survive crashes,
`abort()` and sanitizer reports.

### 8.7 User-facing API

```cpp
// reading
int n; std::string s;
cp::io::read(n, s);                           // types deduced from references
std::vector<long long> a(n); cp::io::read(a); // reads a.size() elements
auto k      = *cp::io::read<int>();           // declare-and-read: a read_result<int>
auto [h, w] = *cp::io::read<int, int>();      // tuple → structured bindings
cp::io::readln(n);                            // reads n, then drops the rest of its line (with the '\n')
cp::io::getline(line);                        // the raw line, spaces kept; other readers: r.getline(line)
while (cp::io::read(x)) { … }                 // every read returns a read_result: true on success
while (auto v = cp::io::read<int>()) { … }    // until end of input, no variable first
if (auto r = cp::io::read(a, b); !r && r.error() == cp::io::read_error::invalid) { … }
cp::io::in.eof();                         // true once every byte of the input has been read

// writing: defaults, or formatters per call (§8.8)
cp::io::write(a, b);                          // separated by " "
cp::io::writeln(n, v);                        // ranges: range_sep per nesting level
cp::io::writeln(cp::io::precision(3), x, y);  // a formatter governs the arguments after it
cp::io::writeln("ans:", cp::io::range_sep(",")(v));   // or decorates only the values it wraps

// debugging (§10)
cp::io::dump(n, v);                           // [main.cpp:12] 3 | [1, 2, 3] on io::err, under CP_CHECKS

// writing: std::format-style specs, as writer member functions
cp::io::out.print("{} {:.12f}\n", name, x);
cp::io::out.println("{:03} {}", n, v);

cp::io::out.flush();
```

Semantics:

- **Supported pattern: `using namespace std;` together with
  `using namespace cp;`** **[owner]**. No cp free function may share a name
  and a viable call shape with a std function. In particular, formatted
  output is `cp::io::out.print`/`println` (member functions, so they cannot
  collide with C++23 `std::print`/`println`), not free functions
  **[my call]**. The test suite includes a "both namespaces" translation unit
  at every language level that calls every public cp function unqualified.
- `read(char&)` skips leading whitespace (as `std::cin >> c` does).
- Reading into a `const` object or a temporary is a compile error with a
  `static_assert` message.
- `std::vector<bool>` is handled by its own path (proxy references).
- `bool` is written as `0`/`1` unless the `bool_alpha` setting is on.
- Layout by nesting level (§8.8): a range of numbers uses the first
  `range_sep` (`" "`), a range of ranges or records the second (`"\n"`), so
  a `vector<vector<int>>`, a `map` and a `vector<pair>` print one row or
  pair per line **[owner, 2026-11-07]**.
- **Members and free functions [owner, 2026-11-07]:** `reader`/`writer`
  members (`read`, `readln`, `getline`, `write`, `writeln`) are the API; the free `cp::io::read`/`readln`/`getline`/
  `write`/`writeln` are wrappers on `cp::io::in`/`cp::io::out` only. Other
  streams use their members (`f.read(x)`), as with `std::ifstream`.
- **`read`, `readln`, `getline` [owner, 2026-11-07]:** `read` skips all
  whitespace, newlines included; `readln(args...)` reads like `read`, then
  drops the rest of the current line with its `\n` (`readln()` just skips
  to the next line), so `readln(n); getline(s);` gets the next line;
  `getline` returns the raw line with its spaces. Leftover tokens dropped by
  `readln` are not reported **[my call]**.
- Parse errors, EOF-while-reading and overflow are diagnosed under
  `CP_CHECKS` (§10); in release builds they are undefined-but-harmless
  (value unspecified, no crash, no hang).

### 8.8 Formatters **[owner, 2026-11-07]** (supersedes the 2026-11-06 options design)

**1. Formatters (decorators), per call only.** Streams hold no settings:
every call starts from the defaults in the table below, and every setting
is a formatter object in `cp::io` passed to the call that needs it.

```cpp
io::writeln(f1, x1, x2, f2, x3, x4);         // f1 → x1, x2; f2 → x3, x4
io::writeln(x0, f1, x1, io::with(), x2);     // x0, x2 with the defaults
io::writeln("ans:", py(v), x);               // py(v): only v is decorated
io::read(io::base(16), a, b, io::with(), c); // a, b hex; c decimal
auto h = *io::read<int>(io::base(16));       // value form
constexpr auto P = io::precision(12);        // a setting wanted everywhere: a constant passed each time
auto py = io::range_sep(", ") | io::range_bracket("[", "]") | io::field_bracket("(", ")");
io::with()                                   // the identity: the defaults
io::with(a, b, c)                            // == a | b | c; arguments may be formatters or presets
```

- **Positional:** a formatter argument governs the arguments after it, up
  to the next formatter argument or the end of the call. Formatters
  **replace** each other: `f2` means defaults + `f2`, so `io::with()`
  restores the default output. Layering is explicit: `f1 | f2`.
- **Wrapped:** `f(values...)` returns a **formatted value** (references to
  the values plus `f`) that decorates only those values, on top of the
  formatter governing it, so `writeln(a, py(v), b)` needs no reset. It
  holds references: use it within the full expression, as with
  `std::forward_as_tuple`. Formatters are plain values, safe to keep.
- `f | g` applies `f`, then `g`: on the same field the right one wins;
  `with() | f` behaves as `f`.
- The separator before an argument is the `arg_sep` of the formatter that
  governs that argument.
- Implementation: an expression template. `|` concatenates the layers into
  one flat `formatting<Layers...>` (the type is not called `formatter`, which would clash with `std::formatter`); the reader and writer build the active
  settings on the stack (defaults, then one inlined store per layer) and
  pass them down by `const&`. With no formatter the settings are
  compile-time defaults, so the plain path keeps no base/width/sign
  branches. No virtual calls, no allocation, no save/restore.
- A user `cp_write` hook that calls `w.write(member)` inherits the active
  settings (the writer tracks them during a call; not public). Hooks cannot
  query settings.
- A formatter applied to the wrong direction (e.g. `range_sep` on a read)
  is a compile error with a `static_assert` message.
- No `set`, no `options`: nothing persists between calls.

**2. The settings**

| Formatter | Direction | Default | Effect |
|---|---|---|---|
| `base(b)` | both | 10 | integer base, 2–36 |
| `float_format(f)` | both | read: `general`; write: `fixed` | `std::chars_format` |
| `precision(p)` | write | 9 | digits after the point; negative: shortest round-trip |
| `width(w)`, `fill(c)`, `show_sign(b = true)` | write | 0, `' '`, off | integer padding and `+` |
| `bool_alpha(b = true)` | write | off | `true`/`false` instead of `1`/`0` |
| `skip_ws(b)` | read | on | whether reading a `char` skips whitespace |
| `arg_sep(s)` | write | `" "` | between the arguments of one call or formatted value |
| `range_sep(s1, s2, …)` | write | `(" ", "\n")` | separator inside a range of nesting level 1, 2, …; deeper levels reuse the last; any number may be given |
| `field_sep(s)` | write | `" "` | between the fields of every tuple-like and plain struct |
| `range_bracket(open, close)` | write | none | around every range |
| `field_bracket(open, close)` | write | none | around every tuple-like and plain struct |
| `quote(b = true)` | write | off | strings in `"…"`, chars in `'…'`, with `\"`, `\\`, `\n`, `\t` escaped |
| `field_names(b = true)` | write | off | plain structs as `name: value` where names are available (§8.10) |
| `wrap(w)` | write | 0 (off) | a range or record wider than `w` breaks one element per line, indented two spaces per level (pprint style) |

- **Nesting level:** numbers, strings, chars, bools and hooked user types
  are level 0; a range is one above its elements; a tuple-like or plain
  struct is one above its highest field. `std::array` is a range.
- No name collides with a std function of the same call shape (spec §8.7):
  `fixed`, `scientific`, `hex` are deliberately not used.

**3. Format specs** in `cp::io::out.print` / `cp::io::out.println` (§8.9). These follow
`std::format` semantics exactly and do **not** consult formatters
**[my call]**: `{}` for a double prints the shortest round-trip form, as
`std::format` does. Formatters govern `cp::io::write`/`writeln` only. Default
floating output for `write`/`writeln` is fixed with 9 digits after the
decimal point.

Explicitly **not** provided: settings stored in streams (`set`,
`options`, per-stream setters), anything that carries a setting from one
call to the next, a separate `join`, separators chosen by element type,
type-keyed bundles.

### 8.9 Format strings

- Spec language: a subset of `std::format`: `{}`, `{n}` (indexed), fill,
  align (`<`, `>`, `^`), sign, `0`, width, `.precision`, types
  `d x X b o f e g c s`. Unsupported specs are errors (compile-time where
  possible, `CP_CHECKS` otherwise).
- Implementation per language level:

  | Level | Mechanism | Errors | Runtime parsing |
  |---|---|---|---|
  | C++17 | runtime parse | runtime under `CP_CHECKS` | one scan per call |
  | C++20+ | `cp::io::format_string<Args...>` with `consteval` constructor; parses at compile time and **stores a segment table** (literal spans + argument slots + parsed specs) in the object | compile time | none: loop over table |

- No `"…"_f` literal **[my call]**: it would be a second way to do the same
  thing, and its C++17 form is a GCC/Clang-only extension that MSVC rejects.
- `cp::io::write`/`writeln` remain the zero-parse path in portable C++17.
- Format specs apply to built-in types. User types support only `{}`; a
  non-empty spec on a user type is an error.

### 8.10 Extending I/O to user types

There is **one** public extension mechanism **[my call]**: user types gain
I/O support by defining, in the type's own namespace:

```cpp
void cp_read(cp::io::reader&, T&);
void cp_write(cp::io::writer&, T const&);
```

found by argument-dependent lookup; no need to reopen `namespace cp`.
`codec<T>` is internal (it implements the built-in categories). cp I/O never
uses `operator<<`/`operator>>` or `std::formatter`; those exist only so cp
types work with iostreams and `std::format` (§7.4).

**Dispatch order**, first match wins:

0. Formatter arguments and formatted values (§8.8) **[owner, 2026-11-07]**:
   built-in kinds, not hooks, because they change the active settings,
   which hooks never do.
1. ADL `cp_read`/`cp_write` hook.
2. Exact built-ins: `bool`, `char`, string types (`std::string`,
   `std::string_view` and `char const*` for output, `char[N]` for output).
3. Arithmetic types (integers including `__int128`, floating point).
4. Ranges (`begin`/`end`), including `std::array` and `std::vector<bool>`.
5. Tuple-like (`std::tuple_size` specialised): `std::pair`, `std::tuple`.
6. Aggregates (automatic, below).

Anything matching none is a compile error naming the type.

Aggregates (plain structs) get automatic I/O with no user code:

- C++17/20/23: Boost.PFR technique (field count by brace-init probing, up
  to a fixed field limit, unpacked via structured bindings).
- C++26 with `__cpp_impl_reflection`: reimplemented with reflection, no
  field limit. Same user-facing behaviour.

**Field names** for the `field_names` setting **[owner, 2026-11-07]**:
reflection (`std::meta::identifier_of`) when `__cpp_impl_reflection`; else,
when `__cpp_nontype_template_args >= 201911L` (C++20: pointers to
subobjects as template arguments), the `__PRETTY_FUNCTION__` trick: a
pointer to each field of a declared-only `extern` object is a template
argument, and the name is cut from the function's signature at compile
time; else (C++17, verified impossible there on GCC and Clang) fields print
without names.

---

## 9. Compatibility backports in detail

### 9.1 C++20

`bit_cast`, `countl_zero`, `countl_one`, `countr_zero`, `countr_one`,
`popcount`, `bit_width`, `bit_floor`, `bit_ceil`, `has_single_bit`,
`rotl`, `rotr`, `endian`, `source_location`, `span` (dynamic and static
extent), `cmp_equal`, `cmp_not_equal`, `cmp_less`, `cmp_greater`,
`cmp_less_equal`, `cmp_greater_equal`, `in_range`, `midpoint`, `ssize`,
`remove_cvref_t`, `type_identity`, `is_constant_evaluated`.

### 9.2 C++23

`unreachable`, `to_underlying`, `byteswap`. `expected` is not provided **[owner]**:
contest code aborts rather than recovering, and no module needs it yet; it may
return later as a `detail` type if one does.

### 9.3 C++26

`inplace_vector` (alias when `__cpp_lib_inplace_vector`; libstdc++ 16
already provides it), `saturating_add`, `saturating_sub`, `saturating_mul`, `saturating_div`,
`saturating_cast` (final C++26 names; the draft names `add_sat`… are not provided).

### 9.4 Ranges

Only `cp::views::iota` and `cp::views::reverse` are backported for C++17
**[my call]**, enough for `std::partition_point(cp::views::iota(lo, hi), pred)`.
Under C++20, `cp::views` is a namespace alias for `std::views`. Both names
then denote the same namespace, so the two using-directives don't make
`views::` ambiguous. Everything else waits for C++20.

---

## 10. Checks and diagnostics **[owner, 2026-11-06]**

- **`CP_CHECK(cond)`**: the user-facing check, and one of cp's two macros
  (the owner allowed macros here because a function always evaluates its
  arguments). Under `CP_CHECKS`, a false `cond` stops the program with
  `file:line: function: check failed: cond`. Otherwise `cond` sits inside
  `sizeof`: it is type-checked, counts as a use of its variables, and is
  never evaluated. Commas inside `cond` are fine (`__VA_ARGS__`).
- **`debug::check(bool cond, std::string_view msg, source_location = current())`**:
  the function behind `CP_CHECK`, used directly by the library when it has a
  computed message (span bounds, `static_vector`, wide-integer division,
  bad input). Its body exists only under `CP_CHECKS`; being a function, it
  evaluates its arguments either way, and the optimiser deletes conditions
  without side effects (checked: even an `is_sorted` loop disappears at -O2).
- **One report path:** `detail::report_failure` flushes `io::out`, writes
  the report to `io::err`, and calls `std::abort`. No C stdio, no
  `fflush(stdout)`: solutions that mix in `printf`/`cout` handle that
  themselves. `std::terminate` was considered and dropped: a terminate
  handler gives no benefit today.
- **`io::dump(a, b, ...)`** **[owner, 2026-11-07]** (replaces `CP_DBG`,
  which is removed): a function in `io/dump.hpp`. Under `CP_CHECKS` it
  flushes `io::out`, then writes `[main.cpp:12] <a> | <b>` and `\n` to
  `io::err`; `dump()` writes `[main.cpp:12]`. The location comes from
  `detail::located_arg` (zero arguments: a defaulted `source_location`).
  Values start from the dump preset instead of the defaults: `arg_sep(" | ")`,
  `range_sep(", ")`, `range_bracket("[", "]")`, `field_sep(", ")`,
  `field_bracket("{", "}")`, `bool_alpha()`, `float_format(general)` with
  `precision(-1)`, `quote()`, `field_names()`, `wrap(80)`. Formatter
  arguments (`dump(io::precision(3), d)`) and formatted values apply on top
  of the preset. Without `CP_CHECKS` its body is empty: the call
  compiles to nothing, but arguments with side effects are still evaluated
  (accepted by the owner; the expression text a macro could print is given
  up too). `CP_CHECK` stays the only contestant macro.
- **Errors are values [owner, 2026-11-06]:** every read returns
  `io::read_result<T>` = `util::result<T, io::read_error>` with
  `read_error::{end_of_input, invalid, out_of_range}`; the reader keeps only
  the last error (sticky until `clear()`, like `cin`) and leaves a bad token
  unread, so `in.token()` shows it and `in.line()` (under `CP_CHECKS`) gives
  its line. The free function `io::read` reports `invalid`/`out_of_range` at
  the caller's line (through `detail::located_arg`, whose first parameter
  carries the caller's `source_location`); `end_of_input` is returned, never
  reported, since it ends read loops. A reader you construct yourself never
  aborts.
- **Layering:** `io/streams.hpp` (`in`, `out`, `err`) → `debug/check.hpp`
  → `io/io.hpp` (free functions) → `io/dump.hpp`. Library preconditions
  in `num`, `ds` and `views` therefore pull in the streams.
- Reports name the file without its directory and the line; the C++17
  `source_location` backport keeps `column()` (0 on GCC) for parity with
  `std::source_location`, but cp does not print it.

---

## 11. Integer aliases

`cp/aliases/aliases.hpp`, namespace `cp::aliases` (not included by `all.hpp`,
not used by library internals) provides `i8 … i64, u8 … u64, isize, usize` **[owner]**, `i128`/`u128` (using-declarations of
`cp::num::i128/u128`) and literal suffixes `_i32`, `_i64`, `_i128`, `_u32`, `_u64`, `_u128`, `_iz`, `_uz`; the 128-bit literals
live only here. No suffixes below 32 bits: arithmetic promotes them to `int` **[owner, 2026-11-07]**. `_uz` is
tagged `cp:since c++23` for removal (C++23 has the built-in `uz` suffix).
Fixes the existing bug where `u16` was `int16_t`.

---

## 12. Algorithm modules

Every module follows §6.3 (namespace `cp::<module>`, directory `cp/<module>/`,
umbrella `<module>.hpp`, names that never clash with std's) and §7, and gets
a test file and, where performance matters, a benchmark. Before a module is
written, the owner is asked whether contestants use it and what to call its
names; the names below are working names. **[owner, 2026-11-06]** `num` holds
number types (integers of any width, modint); `math` holds the functions on
numbers (integer helpers, number theory, combinatorics). All of §12.1's
`math` functions and §12.2 item 9 are wanted, used now or not; each one is
explained to the owner (what, how, why) when it lands.

- **`num::uint_t<N>` / `num::int_t<N>` [owner, 2026-11-06]:** fixed-width
  integers for any power-of-two `N` (8–64 → built-in, 128 → `u128`/`i128`,
  wider → `detail::double_uint`/`double_int`). Overflow wraps at 2ᴺ, as for
  built-in types. Complete: literals wider than 128 bits, `bits::` functions,
  `num::cmp`/`*_sat`/`midpoint`, I/O, tests and a benchmark. A growing
  big integer is not planned.

### 12.1 Ported from the current template, with fixes (Plan 5)

| Module | File | Requirements |
|---|---|---|
| `cp::num` | `num/modint.hpp` | **Done 2026-11-07.** `modint<M>` for any `1 ≤ M < 2⁶⁴` **[owner, 2026-11-07]** (`u32` below 2³², else `u64` with a 128-bit product; `% M` by a constant, which the compiler turns into a multiply) and `dyn_modint<Id>` below 2³² (Barrett reduction, one static modulus per `Id` **[owner, 2026-11-07]**, 998244353 until `set_mod`). Explicit constructor from any integer, normalising negatives. Mixed `modint op integer` operators and `==`. `pow` (negative exponents invert), `inv` (extended Euclid; checked error if not invertible), unary `-`, `++`/`--`. No bitwise operators, no ordering. `std::hash` and cp's I/O; no iostreams or `std::formatter` **[owner, 2026-11-07]**. Aliases `modint998`, `modint1e9_7`. |
| `cp::num` | `num/consts.hpp` | **Done 2026-11-07.** Namespace `cp::num::consts` (not `numbers`: ambiguous with `std::numbers` under `using namespace std`). The 13 `std::numbers` constants and `_v<T>` templates (using-declarations from C++20); `tau`, `inf_v<T>`/`inf` (`max()/2`, infinity for floating point), `mod998`, `mod1e9_7`, `mod1e9_9`, `mersenne61`, `ntt_root`, all `int64_t` **[owner, 2026-11-07: short names]**. |
| `cp::math` | `math/integer.hpp` | `floor_div`, `ceil_div` correct for negative operands, no `x + y - 1` overflow. `isqrt`, `icbrt`. `power` for any type with `*` (not `pow`, which would clash with `std::pow`). `pow_mod(a, e, m)` with 128-bit intermediate (`num::u128`). |
| `cp::math` | `math/prime.hpp` | Deterministic Miller–Rabin for 64-bit (`is_prime`, false for n < 2), linear sieve with smallest-prime-factor table, `divisors`, factorization. |
| `cp::algo` | `algo/search.hpp` | Search for the first `true` of a monotone predicate over an integer range (`lo + (hi - lo) / 2`, never evaluates outside `[lo, hi)`) and over a floating range (fixed iteration count). Not named `partition_point` or `lower_bound` (std names). |
| `cp::ds` | `ds/dsu.hpp` | Iterative find with path compression, union by size, `size(x)`, `same(a, b)`, `count()` maintained in O(1). |
| `cp::str` | `str/split.hpp` | `split`, `trim` (whitespace-aware). |

### 12.2 New modules, in priority order **[my call]** (Plan 6 onward)

1. `cp::math`: `math/combinatorics.hpp` — factorial / inverse factorial tables, `binom`.
2. `cp::algo`: `algo/compress.hpp`, `chmin`/`chmax`.
3. `cp::ds`: `ds/fenwick.hpp`.
4. `cp::ds`: `ds/sparse_table.hpp`.
5. `cp::ds`: `ds/segment_tree.hpp` (generic monoid), then lazy segment tree.
6. `cp::graph`: CSR adjacency, BFS, Dijkstra, topological sort, LCA (binary lifting).
7. `cp::str`: prefix function, Z-function, rolling hash mod 2⁶¹−1.
8. `cp::ds`: `ds/hash.hpp` — splitmix64-based anti-hack hash for unordered containers.
9. `cp::math`: `math/numtheory.hpp` — extended gcd, CRT, Pollard rho.

---

## 13. Build, test, benchmark

### 13.1 Build system: xmake

Replaces CMake **[owner]**.

- Modes: `debug` (`-O0 -g`, ASan + UBSan, `-DCP_CHECKS=1`,
  `-D_GLIBCXX_ASSERTIONS`), `release` (`-O2`, matches judges), `releasedbg`
  (`-O2 -g`, no sanitizers; for `perf` and benchmarks).
- `_GLIBCXX_DEBUG` is **not** used: it changes the ABI and breaks linking
  against prebuilt Catch2/Benchmark packages.
- `compile_commands.json` auto-updated into `build/` for the existing clangd
  configuration.
- Targets: `cp` (header-only), `main` (solution), `test_*` (behind
  `--tests=y`), `bench` (behind `--bench=y`), `headers` (compiles each
  header in isolation).
- Language level selectable: `xmake f --cxxstd=17|20|23|26` (`--cxx` is taken: xmake uses it to select the C++ compiler).
- Tasks `xmake bundle`, `xmake migrate`, `xmake prune` (§14), loaded from
  `tools/xmake/` via `includes()`.
- `.vscode/launch.json` updated to point at the xmake output path.

### 13.2 Tests and benchmarks

- **Catch2 v3** for unit tests; xmake's `catch2` package links `Catch2Main`,
  so test files define no `main`.
- **Differential I/O tests**: random integers/floats/strings written by cp
  and parsed back, and parsed by cp vs `std::from_chars`, with reader buffer
  sizes from 7 bytes to 64 KiB to hit every boundary.
- **Google Benchmark** suite: 10⁷ ints and 10⁶ strings in/out, against
  `scanf/printf`, unsynced iostreams, and the legacy reader/writer.
- `perf record -g` on the `releasedbg` benchmark binary.

### 13.3 Matrix

Every test builds and passes at `-std=c++17`, `c++20`, `c++23`, `c++26`
with GCC and Clang (Linux) and MinGW-w64 (run under wine where available).
`xmake matrix` also bundles every example per cell and fails one over the
size limit (§14.2).

---

## 14. Submission and sharing

### 14.1 Tooling as xmake tasks **[owner]**

Bundling, migration and pruning are xmake tasks written in Lua, in
`tools/xmake/` and loaded by `xmake.lua` with `includes()`. There is no
Python and no separate tool. Commands: `xmake bundle`, `xmake migrate`,
`xmake prune`.

The code-aware steps never parse C++ in Lua. They delegate to compilers:

- **Branch resolution** runs the profile's compiler as a preprocessor with
  `-E -P -fdirectives-only -undef -nostdinc` plus the profile's macro
  definitions. Standard `#include` lines are first masked as comments so
  they pass through untouched, then restored. `-fdirectives-only` evaluates
  `#if`/`#define` but leaves code (including macro uses) unexpanded.
  Prototype-checked during design: the inactive branch was dropped, while
  `#include <vector>` and a macro call passed through unchanged.
- **Symbol analysis** (tree-shaking, migration) runs
  `clang++ -fsyntax-only -Xclang -ast-dump=json -Xclang -ast-dump-filter=cp::`
  and reads the result with xmake's built-in `core.base.json` module. The
  filter is essential. Measured during design: an unfiltered dump of a
  10-line file including `<vector>`/`<iostream>` is 252 MB, the filtered
  dump 0.8 MB, and the whole legacy template filtered is 6.5 MB.
- **Clang is required for analysis only**, even when the judge target is
  GCC. Without `clang++` on `PATH`, `bundle` skips tree-shaking (step 3)
  with a warning, and `migrate` refuses to run.

**Target profiles [owner, 2026-11-06]:** `gcc`, `clang` (Linux judges:
AtCoder and most others) and `mingw` (Windows judges: Codeforces), a table in
`tools/xmake/bundle.lua`; `--std` is separate. The macros are the profile
compiler's own: its predefined ones, plus what `<version>` adds for `--std`
(captured with `-dM -E`), passed with `-include`. `mingw` needs
MinGW-w64's `x86_64-w64-mingw32-g++` (plain `g++` on a Windows host); without
it the bundler stops and says so instead of guessing. Each profile checks
that its compiler targets the judge's system (`__linux__` or `_WIN32` among
its predefined macros), so a Linux-judge bundle is never made by a macOS or
Windows compiler. Its analysis step runs
clang with `--target=x86_64-w64-mingw32`, which reads MinGW's headers. The local compiler may be newer than the judge's:
a library feature it has and the judge lacks picks the wrong branch, so
compile-check a new standard on the judge once.

### 14.2 `xmake bundle`: include only the code the solution needs

```
xmake bundle [--profile=gcc|clang|mingw] [--std=c++20] [--checks=0|1] [--sample=in.txt]   # writes build/submit.cpp
```

Built 2026-11-06, pruning added 2026-11-07 (`tools/xmake/bundle.lua` runs the
stages in `tools/xmake/modules/bundler.lua`):

1. **Inline:** replaces each reached `#include "cp/…"` with the header's
   contents inside a `CP_BUNDLE_GUARD_n` guard, inlining it again at a later
   include until one copy sits outside every `#if` (a false `#if` would drop
   its only copy). Other `#include` lines are masked line by line. The
   solution's code after its last cp include is kept exactly as written.
2. **Resolve branches:** the profile's compiler runs
   `-E -P -fdirectives-only -dD -ffreestanding -include <library macros>
   -DCP_CHECKS=<n>` over the solution's head and the inlined headers. Every
   `#if` is evaluated with the judge's macros; code and macro uses stay
   unexpanded; `#define`s survive (`-dD`), and the library's feature macros
   are filtered back out.
3. **Strip:** comments (outside string, character and digit-separated number
   literals), blank lines, indentation, alignment runs, repeated standard
   `#include`s. `--checks=1` also prepends `#define _GLIBCXX_ASSERTIONS 1`.
4. **Tree-shake by name** (clang required for analysis only): the filtered
   AST dump (`-ast-dump=json -ast-dump-filter=cp::`) gives every
   namespace-scope declaration in `cp` with its exact byte range. Roots are
   all text outside those declarations. A declaration is kept when its name
   appears in kept text; this keeps every overload, specialization and
   deduction guide of a used name, and a kept class whole. An operator is
   kept when its text names a kept declaration (ADL finds it, never its
   name). A specialization, of a cp template or of a std one
   (`std::hash<cp::util::result<T, E>>`), is dropped only when its pattern
   names a cp class nothing keeps (it can never be picked); other names in
   it are kept along. A dropped declaration takes its `;` and leading
   `[[attributes]]`; emptied namespaces go, except one still named
   (`using namespace cp::aliases;`).
5. **Prune what nothing instantiates** (same AST, instantiations included;
   an instance's body shares its pattern's byte offsets): a function body no
   instantiation or call reaches becomes `;`; an unreferenced plain function
   or hidden friend is deleted; an `if constexpr` branch no instantiation
   keeps becomes `{}`; a namespace-scope class template never instantiated
   becomes a declaration. Steps 4 and 5 repeat until pruning changes nothing.
   Then `CP_` macros nothing uses are dropped.
6. **Verify, smallest first:** the pruned, then the shaken, then the inlined
   bundle, each compiled with the profile's compiler; with `--sample`, run
   with the unbundled solution (built once) on that input, requiring the same
   output and exit code (`mingw` through wine, skipped without it). A failing
   candidate is saved as `build/failed-<stage>.cpp` with the compiler's errors
   shown, and the next one is tried. Work files go to a fresh temporary
   directory per run.
7. **Size gate:** fails above 65535 bytes, listing the largest kept
   declarations; nothing is written then.

Measured 2026-11-07 over every example (gcc C++17, gcc C++26 with checks,
clang C++20, mingw C++23; all with the same output and exit code as
unbundled): 12–41 KB. The io programs are 23–33 KB (46 KB with shaking only),
`modint` 35–41 KB (74 KB before pruning, over the limit). About 4 s per
bundle, mostly clang's AST dumps.

- Solutions may include `cp/all.hpp`; step 4 removes what isn't used. The
  template still includes only `cp/io.hpp`, so it stays fast to compile
  locally.
- `dist/cp.hpp` = `all.hpp` after step 1 and comment stripping only, with no
  branch resolution (it must work on any compiler), committed. It is not
  held to the 64 KB limit.

### 14.3 `xmake migrate`: move solution code between standards, up or down

```
xmake migrate --to=c++20 solution.cpp   # upgrade
xmake migrate --to=c++17 solution.cpp   # downgrade
```

- **Alias table:** generated from the `// cp:since c++NN` tags in
  the module headers. Each entry is a cp name, its std name, the std header it needs,
  and the standard that introduced it. No hand-maintained list.
- **Upgrade:** every reference that the clang AST dump resolves to a cp alias whose
  standard is at or below `--to` is rewritten to the std name
  (`cp::ds::span` → `std::span`). The needed std header is added. Unqualified
  uses through `using namespace` are handled, because the rewrite works on
  resolved references, not text.
- **Downgrade:** every reference to a std facility newer than `--to` that
  has a cp backport is rewritten to the cp name (`std::span` → `cp::ds::span`).
  The corresponding `cp/<module>/…` include is added.
- **What it cannot do:** rewrite *language* features that have no
  library-level equivalent, such as concepts, designated initializers, or
  `std::ranges` views without a cp backport. After rewriting, `migrate`
  compiles the file at the target standard and reports each remaining error
  with its location and the feature responsible. It does not try to rewrite
  those.
- Edits go to the file in place only with `--in-place=y`; otherwise the tool
  writes a diff to stdout.

### 14.4 `xmake prune`: raise the library's floor

```
xmake prune --min=23
```

- Deletes the fallback side of every `cp:since` branch at or below the new
  minimum (§6.5), sets `CP_MIN_CXX`, and marks the removed aliases
  `[[deprecated]]` for one release (§6.3).
- One-way on the library source. It refuses to run on a dirty git tree and
  creates the tag `cp-pre-prune-c++NN` first, so the previous state can be
  restored.
- A downgraded *submission* never needs the library to be un-pruned while
  the target is at or above `CP_MIN_CXX`. Older targets than that are
  served from the tag.

## 15. Solution template (`main.cpp`)

```cpp
#include "cp/io.hpp"
using namespace cp;

void solve() {
}

int main() {
    for (auto t = read<int>(); t--;) solve();
}
```

---

## 15a. User documentation **[owner asked; form is my call]**

Users must be able to adopt the library, the xmake tasks and the migration
workflow from the docs alone.

- **`README.md`** (repo root): what the library is, requirements (compilers,
  xmake, clang for analysis), a 5-minute quick start (clone → `xmake` →
  edit `main.cpp` → `xmake bundle` → submit), and links into the guide.
- **Built 2026-11-06 [owner: Doxygen only, published to GitHub Pages]:**
  `xmake docs` builds one site from `README.md` (the home page), the guide
  and the API reference with Doxygen and the doxygen-awesome theme (pinned
  v2.3.4 in `docs/doxygen/`); `.github/workflows/docs.yml` deploys it on
  every push to `master`. Guide code blocks marked `<!-- example: x.cpp -->`
  are copied from `examples/x.cpp` by `xmake docs`, and `xmake docs --check`
  fails when one differs; the examples compile with the tests in every
  matrix cell.
- **`docs/guide/`**: plain Markdown with a `SUMMARY.md` table of contents
  (the GitBook layout). It renders on GitHub as is. Pages:
  1. Getting started: install, template, build modes, checks (`CP_CHECKS`).
  2. Reading and writing: every `read`/`write`/`print` form, formatters
     and settings, interactive problems, I/O for your own types.
  3. Checks and debugging: `CP_CHECK`, `io::dump`, what checked builds catch.
  4. Language levels: what `cp::` aliases exist per standard, `CP_MIN_CXX`,
     each backported name tagged *user* or *internal* (internal names stay public
     but the guide shows contestants only the user ones),
     and `using namespace std;` with `using namespace cp;`.
  5. Tooling: `xmake bundle` (profiles, tree-shaking, size gate, fallbacks),
     `xmake migrate` (what it rewrites, what it reports), `xmake prune`.
  6. Testing and benchmarking: tests, benchmarks, `perf`.
  7. One page per module (`modint`, `dsu`, …): interface, complexity,
     preconditions, example.
  8. Contributing: header rules (§4), fidelity rules (§7), version tags (§6.5).
- **Examples are compiled.** Every code example in the docs lives in
  `examples/*.cpp`, built by an xmake target at every language level, and is
  embedded in the Markdown. Docs can't drift from the API without breaking
  the build.
- **Docs ship with the code.** Each build-order step (§17) includes its
  guide pages; a step is not done until its docs are written.

## 16. Migration from the current `main.cpp`

The current template's known defects (missing `<cassert>`, broken float and
`bool` I/O, `readln` overflow, `eof()` before first read, `char` not skipping
whitespace, `std::array<char, N>` ambiguity, `mint` negatives and division,
`inplace_vector` double destruction / bounds / assignment / `resize`,
binary-search out-of-range evaluation, negative `floor`/`ceil`,
`is_prime(-7)`, buffered `stderr` losing debug output, Windows `\r\r\n`,
`vector<bool>` reads, `u16` signedness) are **not patched in place**; each
is covered by the new design and by a regression test in the new suite.

## 17. Build order

1. `config.hpp` and the C++17–26 backports, now in their modules (`bits`,
   `num`, `ds`, `views`, `traits`, `debug`), with the header-isolation check
   and the 17/20/23/26 matrix. **Done (Plan 1).**
2. `sys/` input sources and output sinks, `io/` (reader, writer, codecs,
   options, format engine), tests and benchmarks, `xmake bundle` (inlining,
   comment stripping, `--checks`), `xmake iobench`, the new `main.cpp`.
   **Done (Plan 2; `print`/`println` deferred).**
3. Plan 3: `CP_CHECK`, `debug::check`, `CP_DBG` (§10), replacing
   `detail/precondition.hpp`. **Done.** `CP_DBG` later replaced by `io::dump`.
4. Plan 4: the rest of `xmake bundle` (judge profiles, `#if` resolution,
   symbol tree-shaking), `dist/cp.hpp`, `xmake migrate`, `xmake prune`,
   Wine testing (MinGW builds are in the matrix since 2026-11-06). Bundle
   pipeline with dead-code pruning **done 2026-11-07**; `dist/cp.hpp`,
   `migrate`, `prune` remain.
4b. I/O formatters, `readln`, `io::dump` (§8.7, §8.8, §10), plan
   `docs/superpowers/plans/2026-11-07-io-formatters.md` **[owner, 2026-11-07]**.
5. Plan 5: port existing modules (§12.1).
6. Plan 6 onward: new modules (§12.2).

Each step includes its documentation (§15a); `README.md` is written in step 1
and extended as features land.

## 18. Left to the builder

- Exact internal structure of the format-spec engine and segment table.
- The SWAR digit-validation and combine bit tricks.
- Field limit for the C++17 aggregate codec (suggested 16).
- Benchmark input generation details.
- Internal naming not listed in this document.

## 19. Open items for the owner

- A paste-ready template (`dist/template.cpp`: a pre-shaken bundle of io,
  aliases, `CP_MAIN` and `dump`, rebuilt and C++17-checked by CI) for judges
  where `xmake bundle` cannot run. Deferred by the owner on 2026-11-07.
