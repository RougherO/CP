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

- Supporting compilers older than GCC 11, Clang 14, MSVC 19.30 **[my call]**.
- 32-bit targets. Every supported target is 64-bit **[owner]**.
- Full `std::ranges` or full `std::format` reimplementation in C++17. Only
  the subset listed here is backported.
- Global-scope convenience: the library lives in `namespace cp`; solutions
  opt in with `using namespace cp;`.

**Anti-goals** (the design has failed if any of these happen)

- User-facing macros for solving (no `#define int long long`, no loop/debug
  macros, no `ll`/`vi`/`pb`) **[owner]**.
- Shorthand aliases for things that already exist in C++17 (no `vec`, no
  `cp::vector`, no `cp::string`) **[owner]**.
- Too many ways to do the same thing. Each need has one obvious mechanism
  **[owner]**.
- Hidden global formatting state that a call can silently change.

## 3. Constraints

- **Language baseline:** compiles with `-std=c++17`. Must also compile, with
  identical behaviour, at `-std=c++20`, `c++23` and `c++26`.
- **Compilers:** GCC ≥ 11 and Clang ≥ 14 are release requirements. MSVC ≥ 19.30
  is best-effort: the code paths exist and are kept compiling when an MSVC
  toolchain is available, but MSVC is not a release gate **[my call]**.
- **64-bit only** **[owner]**. `static_assert(sizeof(void*) == 8)` in
  `config.hpp`. On MSVC (no `__int128`), 128-bit products use `_umul128`/`_umulh`.
- **Platforms:** Linux (most judges, local dev), Windows (Codeforces runs
  Windows with GCC/MSVC), macOS (local dev only).
- **No `bits/stdc++.h` inside library headers.** Each header includes the
  standard headers it uses. (`bits/stdc++.h` does not exist on MSVC or
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
│   ├── config.hpp            the only file that defines macros (see §5)
│   ├── compat/               post-C++17 std facilities, one file per std header
│   │   ├── version.hpp       includes <version> when available; feature macros
│   │   ├── meta.hpp          is_detected, remove_cvref_t, type_identity
│   │   ├── bit.hpp           <bit>: bit_cast, countl_zero, countr_zero, popcount,
│   │   │                     bit_width, bit_floor, bit_ceil, has_single_bit, byteswap, endian
│   │   ├── utility.hpp       <utility>: cmp_equal/cmp_less/…, in_range,
│   │   │                     to_underlying, unreachable
│   │   ├── numeric.hpp       <numeric>: midpoint, saturating_add/sub/mul/div, saturating_cast
│   │   ├── iterator.hpp      ssize
│   │   ├── source_location.hpp
│   │   ├── span.hpp
│   │   ├── inplace_vector.hpp
│   │   └── ranges.hpp        minimal views: iota, reverse (see §9.3)
│   ├── sys/                  only place OS headers are included
│   │   ├── input_source.hpp  mmap / read(2) / ReadFile
│   │   ├── output_sink.hpp   write(2) / WriteFile
│   │   └── cpu.hpp           cpu_supports(cpu_feature), prefetch
│   ├── int128.hpp        cp::i128/u128 (__int128, else detail::double_int<u64>), _i128/_u128 literals
│   ├── wide.hpp          widen_t (any width), wide<T>{low, high}, add_wide/sub_wide/mul_wide [owner]
│   ├── intrin/
│   │   └── simd.hpp          opt-in SSE4.2/AVX2/NEON helpers
│   ├── io/
│   │   ├── options.hpp       per-type option structs, input_options/output_options
│   │   ├── codec.hpp         codec<T> customization point + built-in codecs
│   │   ├── parse.hpp         integer (SWAR) and float parsing
│   │   ├── format.hpp        integer/float formatting, format-spec engine
│   │   ├── reader.hpp
│   │   ├── writer.hpp
│   │   └── io.hpp            cp::in, cp::out, cp::err and free functions
│   ├── check.hpp             cp::check
│   ├── debug.hpp             cp::dbg
│   ├── aliases.hpp           optional short integer aliases (see §11)
│   ├── math/ ds/ graph/ string/ …   algorithm modules (see §12)
│   └── all.hpp               includes every public header
├── main.cpp                  solution template (thin)
├── tests/                    Catch2, one test_<module>.cpp per header
├── bench/                    Google Benchmark
├── stress/                   gen.cpp, brute.cpp, stress script
├── tools/
│   ├── xmake/                xmake tasks: bundle, migrate, prune (§14)
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

`config.hpp` is the **only** header that defines macros. All macros are
prefixed `CP_`. They exist only where the language offers no alternative:
version detection, compiler/OS/architecture detection, attributes.

### 5.1 Language level

```cpp
#if defined(_MSVC_LANG) && _MSVC_LANG > __cplusplus
#  define CP_CXX _MSVC_LANG
#else
#  define CP_CXX __cplusplus
#endif
#define CP_CXX17 (CP_CXX >= 201703L)
#define CP_CXX20 (CP_CXX >= 202002L)
#define CP_CXX23 (CP_CXX >= 202302L)
#define CP_CXX26 (CP_CXX >  202302L)   // until the final value is published
#ifndef CP_MIN_CXX
#  define CP_MIN_CXX 201703L
#endif
static_assert(CP_CXX >= CP_MIN_CXX, "cp: language level below CP_MIN_CXX");
```

`CP_CXXnn` are used only for coarse decisions. Individual features are
always detected by their feature-test macro (§6.1).

### 5.2 Detection

| Macro | Notes |
|---|---|
| `CP_CLANG`, `CP_GCC`, `CP_MSVC` | Checked in that order: Clang defines `__GNUC__`; clang-cl defines `_MSC_VER` and is treated as `CP_CLANG`. |
| `CP_OS_LINUX`, `CP_OS_WINDOWS`, `CP_OS_MACOS`, `CP_OS_POSIX` | `CP_OS_POSIX` is set on Linux and macOS. |
| `CP_ARCH_X64`, `CP_ARCH_ARM64` | Any other architecture, and any 32-bit target, is a hard `#error`. |
| `CP_HAS_INT128` | `__SIZEOF_INT128__` defined. |
| `CP_HAS_SSE42`, `CP_HAS_AVX2`, `CP_HAS_NEON` | From compiler-predefined macros (compile-time availability only). |
| `CP_CHECKS` | 1 when `LOCAL` is defined or `NDEBUG` is not; 0 otherwise. Overridable with `-DCP_CHECKS=0/1`. |

### 5.3 Attributes

| Macro | GCC/Clang | MSVC |
|---|---|---|
| `CP_INLINE` | `__attribute__((always_inline)) inline` | `__forceinline` |
| `CP_NOINLINE` | `__attribute__((noinline))` | `__declspec(noinline)` |
| `CP_LIKELY(x)` / `CP_UNLIKELY(x)` | `__builtin_expect(!!(x), 1/0)` | `(x)` |
| `CP_RESTRICT` | `__restrict__` | `__restrict` |
| `CP_TARGET(isa)` | `__attribute__((target(isa)))` | empty |

`[[likely]]`, `[[assume]]` etc. replace these once `CP_MIN_CXX` permits
(tagged per §6.3).

### 5.4 Intrinsics, exposed as functions

Intrinsics are wrapped in `constexpr` functions, never exposed as macros.
Where an intrinsic is not usable in constant evaluation (MSVC
`_BitScanReverse64`, `__popcnt64`, `_umul128`), the wrapper branches on
`cp::is_constant_evaluated()` (`__builtin_is_constant_evaluated`, available in
C++17 mode on all three compilers) and uses a portable loop at compile time.

| Facility | GCC/Clang | MSVC |
|---|---|---|
| `countl_zero`, `countr_zero`, `popcount` | `__builtin_clzll`, `__builtin_ctzll`, `__builtin_popcountll` | `_BitScanReverse64`, `_BitScanForward64`, `__popcnt64` |
| `bit_cast` | `__builtin_bit_cast` | `__builtin_bit_cast` |
| `source_location::current()` | `__builtin_FILE/LINE/COLUMN/FUNCTION` | same builtins (19.26+) |
| `unreachable()` | `__builtin_unreachable` | `__assume(0)` |
| `mulhi`, `mul128` | `unsigned __int128` | `_umulh`, `_umul128` |
| `add_overflow`, `mul_overflow` | `__builtin_add_overflow`, `__builtin_mul_overflow` | `_addcarry_u64` / manual |
| `prefetch` | `__builtin_prefetch` | `_mm_prefetch` |
| `cpu_supports(feature)` | `__builtin_cpu_supports` | `__cpuidex` |

### 5.5 SIMD

- **Opt-in only.** Never enabled for the whole program (judge safety).
- SIMD code lives in individual functions marked `CP_TARGET("avx2")` (or
  similar) and is selected at runtime via `cp::cpu_supports`, with a scalar
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
  `<version>` (`compat/version.hpp` includes `<version>` when
  `__has_include(<version>)`, which GCC 9+/Clang 7+/MSVC provide in C++17
  mode).
- **Language features** are detected with `__cpp_*` macros, e.g.
  `__cpp_concepts`, `__cpp_consteval`, `__cpp_nontype_template_args`,
  `__cpp_explicit_this_parameter`, `__cpp_impl_reflection`,
  `__cpp_contracts`.
- `CP_CXXnn` is never used to decide whether one feature exists.

### 6.2 Three ways a newer-standard feature enters

| Kind | Rule | Examples |
|---|---|---|
| **Alias or backport** (library) | `cp::x` is a using-declaration of `std::x` when the feature macro is defined, otherwise a cp implementation with the same interface. | C++20: `bit_cast`, `<bit>`, `span`, `source_location`, `cmp_*`, `in_range`, `midpoint`, `ssize`, `remove_cvref_t`, `type_identity`, `is_constant_evaluated`. C++23: `unreachable`, `to_underlying`, `byteswap`. C++26: `inplace_vector`, saturating arithmetic. |
| **Additive** (language) | Extra overloads/specializations compiled only when available; the C++17 path keeps working. | C++20: `operator<=>`, `std::formatter<cp::T>` specializations, `std::ranges` concept constraints, `consteval` format-string checking. C++23: multi-index `operator[]` on grid types. C++26: reflection-based aggregate codec, contracts behind `cp::check`. |
| **Internal replacement** | Internals switch when `CP_MIN_CXX` rises; public API unchanged. | `if consteval` replaces `is_constant_evaluated()`; deducing `this` removes const/non-const duplicates; `std::simd` replaces `intrin/simd.hpp` internals; `[[assume]]`/`[[likely]]` replace attribute macros. |

### 6.3 `cp::` alias policy **[owner]**

- `cp::` re-exports **only** standard facilities introduced **after C++17**.
  C++17 facilities (`std::vector`, `std::string`, `std::optional`,
  `std::from_chars`, …) are used as `std::` everywhere and never aliased.
- When `CP_MIN_CXX` reaches the standard that introduced a facility, its
  `cp::` alias is marked `[[deprecated("use std::x")]]` for one release and
  then deleted.

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

1. **Counterpart exists in std** → `cp::x` is indistinguishable: same name,
   template parameters, member typedefs, `constexpr`/`noexcept`/
   `[[nodiscard]]`, complexity guarantees, exception/precondition behaviour.
   When std provides it, `cp::x` *is* `std::x`.
   **Exception:** I/O. cp's formatted output is cp's own fast implementation
   and is never an alias of `std::print`.
2. **No counterpart** (`dsu`, `fenwick_tree`, `segment_tree`, `modint`, …) →
   follow std conventions: snake_case; containers provide `value_type`,
   `size_type`, `size()`, `empty()`, `begin()/end()`, `operator[]`
   (unchecked except under `CP_CHECKS`), `at()` (always checked);
   number-like types behave like built-in arithmetic types.
3. **Algorithms** come as an iterator-pair overload and a range overload,
   both directly in `cp::` **[my call]**. There is no `cp::ranges` namespace,
   because `ranges::x` would be ambiguous when a solution has both
   `using namespace std;` and `using namespace cp;`. Under C++20 the range
   overloads are constrained with `std::ranges` concepts so std views are
   accepted.
4. **Every cp type participates in std customization points** where
   meaningful: `std::hash`, `std::tuple_size`/`get`, `std::numeric_limits`
   (number-like types), `operator<<`/`operator>>` for iostreams,
   `std::formatter` (when `__cpp_lib_format`).
5. **Shared conformance tests:** where a std counterpart exists, one typed
   Catch2 suite runs against both (e.g. `cp::inplace_vector` tests also run
   against `std::inplace_vector` when available).

---

## 8. I/O

The I/O subsystem is the centerpiece. Requirements: never the bottleneck;
correct on file, pipe and interactive input; identical behaviour on Linux and
Windows.

### 8.1 Input source (`sys/input_source.hpp`)

Chosen once, when `cp::in` is first used:

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
  three-multiplication method), scalar tail. Under `CP_CHECKS`, overflow is
  detected via `cp::mul_overflow`/`add_overflow` and reported (§10).
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

`cp::in` is tied to `cp::out`: before the reader performs a refill that may
block (pipe/TTY source), it flushes `cp::out`. With `mmap`/slurped input the
tie never triggers. Interactive solutions never need a manual flush.

### 8.6 Streams

```cpp
namespace cp {
inline reader in;    // stdin
inline writer out;   // stdout
inline writer err;   // stderr, unbuffered (writes through immediately)
}
```

`cp::err` writes through on every call so diagnostics survive crashes,
`abort()` and sanitizer reports.

### 8.7 User-facing API

```cpp
// reading
int n; std::string s;
cp::read(n, s);                           // types deduced from references
std::vector<long long> a(n); cp::read(a); // reads a.size() elements
auto k      = cp::read<int>();            // declare-and-read
auto [h, w] = cp::read<int, int>();       // tuple → structured bindings
cp::getline(cp::in, line);                // whole line, same shape as std::getline
while (cp::read(x)) { … }                 // read returns cp::reader&, which converts to bool
                                          // (false once a read hit end of input), like `while (cin >> x)`
cp::in.eof();                             // std semantics: true after a read hit end of input

// writing: stream options only
cp::write(a, b);                          // separated by output_options::arg_sep
cp::writeln(n, v);                        // ranges: elements separated by range_sep

// writing: std::format-style specs, as writer member functions
cp::out.print("{} {:.12f}\n", name, x);
cp::out.println("{:03} {}", n, v);

cp::out.flush();
```

Semantics:

- **Supported pattern: `using namespace std;` together with
  `using namespace cp;`** **[owner]**. No cp free function may share a name
  and a viable call shape with a std function. In particular, formatted
  output is `cp::out.print`/`println` (member functions, so they cannot
  collide with C++23 `std::print`/`println`), not free functions
  **[my call]**. The test suite includes a "both namespaces" translation unit
  at every language level that calls every public cp function unqualified.
- `read(char&)` skips leading whitespace (as `std::cin >> c` does).
- Reading into a `const` object or a temporary is a compile error with a
  `static_assert` message.
- `std::vector<bool>` is handled by its own path (proxy references).
- `bool` is written as `0`/`1` unless `output_options::boolean.alpha`.
- Nested ranges: inner ranges separated by `\n` (one row per line), elements
  by `range_sep` **[my call]**.
- Parse errors, EOF-while-reading and overflow are diagnosed under
  `CP_CHECKS` (§10); in release builds they are undefined-but-harmless
  (value unspecified, no crash, no hang).

### 8.8 Options **[owner]**

Exactly two mechanisms, nothing else:

**1. Stream-wide options**, set or read in one call.

- Each codec defines its option structs; `cp::read_options<T>` and
  `cp::write_options<T>` are aliases to them. All integer types share one
  struct; all floating types share one.

  | Category | read options | write options |
  |---|---|---|
  | integer | `base` | `base`, `width`, `fill`, `show_sign` |
  | floating | `format` | `format`, `precision` (default 9) |
  | bool | — | `alpha` |
  | char | `skip_ws` (default true) | — |
  | string | — | — |

- Each stream holds one aggregate:

  ```cpp
  struct input_options  { read_options<long long> integer; read_options<double> floating; read_options<char> character; };
  struct output_options { write_options<long long> integer; write_options<double> floating;
                          write_options<bool> boolean; std::string_view arg_sep = " ";
                          std::string_view range_sep = " "; };
  ```

- Get/set follows `std::ios_base::flags()`:

  ```cpp
  auto cur = cp::out.options();                                   // get
  auto old = cp::out.options({.floating = {.precision = 12}});    // set, returns previous
  ```

  C++17 has no designated initializers, so every options struct also offers
  chainable `constexpr` setters returning a modified copy
  (`output_options{}.set_floating(...)`). Structs remain aggregates.

- **Scoped wrapper**: an RAII type that sets options on construction and
  restores the previous options on destruction. **Name to be decided by the
  owner** (working name `cp::scoped_options`).

- Streams hold options only for built-in categories. User I/O hooks
  (§8.10) receive the reader/writer and may query `options()` themselves.

**2. Format specs** in `cp::out.print` / `cp::out.println` (§8.9). These follow
`std::format` semantics exactly and do **not** consult stream options
**[my call]**: `{}` for a double prints the shortest round-trip form, as
`std::format` does. Stream options govern `cp::write`/`writeln` only. Default
floating output for `write`/`writeln` is fixed with 9 digits after the
decimal point.

Explicitly **not** provided: per-call options objects, per-value trailing
options, positional layouts, type-keyed bundles.

### 8.9 Format strings

- Spec language: a subset of `std::format`: `{}`, `{n}` (indexed), fill,
  align (`<`, `>`, `^`), sign, `0`, width, `.precision`, types
  `d x X b o f e g c s`. Unsupported specs are errors (compile-time where
  possible, `CP_CHECKS` otherwise).
- Implementation per language level:

  | Level | Mechanism | Errors | Runtime parsing |
  |---|---|---|---|
  | C++17 | runtime parse | runtime under `CP_CHECKS` | one scan per call |
  | C++20+ | `cp::format_string<Args...>` with `consteval` constructor; parses at compile time and **stores a segment table** (literal spans + argument slots + parsed specs) in the object | compile time | none: loop over table |

- No `"…"_f` literal **[my call]**: it would be a second way to do the same
  thing, and its C++17 form is a GCC/Clang-only extension that MSVC rejects.
- `cp::write`/`writeln` remain the zero-parse path in portable C++17.
- Format specs apply to built-in types. User types support only `{}`; a
  non-empty spec on a user type is an error.

### 8.10 Extending I/O to user types

There is **one** public extension mechanism **[my call]**: user types gain
I/O support by defining, in the type's own namespace:

```cpp
void cp_read(cp::reader&, T&);
void cp_write(cp::writer&, T const&);
```

found by argument-dependent lookup; no need to reopen `namespace cp`.
`codec<T>` is internal (it implements the built-in categories). cp I/O never
uses `operator<<`/`operator>>` or `std::formatter`; those exist only so cp
types work with iostreams and `std::format` (§7.4).

**Dispatch order**, first match wins:

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
**[my call]**, enough for `cp::partition_point(cp::views::iota(lo, hi), pred)`.
Under C++20, `cp::views` is a namespace alias for `std::views`. Both names
then denote the same namespace, so the two using-directives don't make
`views::` ambiguous. Everything else waits for C++20.

---

## 10. Checks and diagnostics

- `cp::check` is a function, not a macro, with two overloads **[owner, 2026-11-04]**:
  - `cp::check(bool cond, std::string_view msg = {}, cp::source_location = current())`
    for cheap conditions. Being a function argument, `cond` is **always
    evaluated**, even when `CP_CHECKS` is 0; only the reporting disappears.
    Side-effect-free conditions (`i < v.size()`) are removed by the optimiser,
    but anything with side effects or opaque calls still runs. Documented as
    "keep it cheap and side-effect free".
  - `cp::check(F&& pred, std::string_view msg = {}, cp::source_location = current())`
    for expensive conditions, where `pred` is a callable returning something
    convertible to `bool`: `cp::check([&] { return is_valid(g); }, "graph")`.
    `pred` is invoked only when `CP_CHECKS` is 1, so with checks off the
    expensive code never runs at any optimisation level.
  - On failure: writes `file:line: function: check failed: msg` to `cp::err`,
    flushes `cp::out`, calls `std::abort()`. Under C++26 contracts
    (`__cpp_contracts`) it maps to `contract_assert` (additive).
- Library preconditions (`operator[]` bounds, `modint` division by zero,
  reading past EOF, parse errors, overflow) use the same reporting, pointing
  at the **user's** call site via a defaulted `source_location` parameter
  on the public API.
- `cp::dbg(args...)`: prints `[file:line] v1 | v2 | …` to `cp::err`.
  Implemented as a class template with a deduction guide so a variadic call
  still receives a defaulted `source_location`. Compiles to nothing unless
  `CP_CHECKS`.
- **No macro for capturing variable names** in `dbg` **[my call]**,
  consistent with the no-macros anti-goal.

---

## 11. Integer aliases

`cp/aliases.hpp` (not included by `all.hpp`, not used by library internals)
provides `i8 … i64, u8 … u64, isize, usize` (`cp::i128/u128` live in `cp/int128.hpp`, always available) **[owner]**,
and literal suffixes `_i64`, `_u64`, `_uz`, `_iz` **[my call]**. `_uz` is
tagged `cp:since c++23` for removal (C++23 has the built-in `uz` suffix).
Fixes the existing bug where `u16` was `int16_t`.

---

## 12. Algorithm modules

### 12.1 Ported from the current template (with fixes)

| Module | Requirements |
|---|---|
| `math/modint.hpp` | `cp::static_modint<M>` and `cp::dynamic_modint` (Barrett reduction via `mulhi`). Explicit constructor from integers, normalising negatives. Value stored as `u32`. Add/sub by conditional subtraction; mul via 64-bit product. Mixed `modint op integer` operators. `pow`, `inv` (extended Euclid; checked error if not invertible), unary `-`. No bitwise operators, no ordering. Integrates with `std::hash`, iostreams, `std::formatter`, `cp` codecs. |
| `math/integer.hpp` | `floor_div`, `ceil_div` correct for negative operands, no `x + y - 1` overflow. `isqrt`, `icbrt`. `power` for any type with `*` (not named `pow`, to avoid clashing with `std::pow`). `pow_mod(a, e, m)` with 128-bit intermediate. |
| `math/prime.hpp` | Deterministic Miller–Rabin for 64-bit (`is_prime`, false for n < 2), linear sieve with smallest-prime-factor table, `divisors`, factorization. |
| `algo/binary_search.hpp` | `partition_point`-style search over integer ranges (`lo + (hi - lo) / 2`, never evaluates outside `[lo, hi)`) and over floating ranges (fixed iteration count). |
| `ds/dsu.hpp` | Iterative find with path compression, union by size, `size(x)`, `same(a, b)`, `count()` maintained in O(1). |
| `string/split.hpp` | `split`, `trim` (whitespace-aware). |

### 12.2 New modules, in priority order **[my call]**

1. `math/combinatorics.hpp` — factorial / inverse factorial tables, `binom`.
2. `util/compress.hpp`, `chmin`/`chmax`.
3. `ds/fenwick_tree.hpp`.
4. `ds/sparse_table.hpp`.
5. `ds/segment_tree.hpp` (generic monoid), then lazy segment tree.
6. `graph/` — CSR adjacency, BFS, Dijkstra, topological sort, LCA (binary lifting).
7. `string/` — prefix function, Z-function, rolling hash mod 2⁶¹−1.
8. `util/hash.hpp` — splitmix64-based anti-hack hash for unordered containers.
9. `math/numtheory.hpp` — extended gcd, CRT, Pollard rho.

Each module follows §7 and gets a test file and, where performance matters,
a benchmark.

---

## 13. Build, test, benchmark

### 13.1 Build system: xmake

Replaces CMake **[owner]**.

- Modes: `debug` (`-O0 -g`, ASan + UBSan, `-DLOCAL`,
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
with GCC and Clang (Linux). MSVC is best-effort (§3): built when a
toolchain is available, failures are bugs but do not block a release.

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

**Target profiles** (`tools/profiles/*.lua`, plain Lua tables) describe a judge target:
compiler executable, `-std=` level, flags, and the predefined macros
(`__GNUC__`, `__cpp_lib_*`, …) obtained by running that compiler with
`-dM -E`. Example names: `gcc13-c++20`, `gcc14-c++23`, `clang18-c++23`.

### 14.2 `xmake bundle`: include only the code the solution needs

```
xmake bundle --profile=gcc14-c++23   # writes submit.cpp
```

1. **Inline:** replaces each reached `#include "cp/…"` with the header's
   contents, once per header. Standard `#include`s are left untouched.
2. **Resolve branches:** evaluates **every** conditional (§14.1 mechanism)
   with exactly the macros the judge would see: the profile's predefined
   macros, `LOCAL` undefined, `NDEBUG` as the judge sets it. That is the
   correct result for a submission, including `#ifdef LOCAL` blocks in the
   solution itself, which drop out.
3. **Tree-shake by symbol** (like include-what-you-use, but for
   declarations): reads the filtered clang AST dump (§14.1) of the result
   and builds a reference graph over namespace-scope declarations in `cp`.
   It starts from everything `main.cpp` references and keeps what is
   transitively reachable. Conservative rules, because some uses are
   implicit:
   - A kept class or class template is kept whole (no member pruning).
   - A kept primary template keeps all its partial and explicit
     specializations and its deduction guides.
   - A reference to one overload keeps every overload of that name.
   - Any function whose parameter types mention a kept type is kept
     (operators, ADL hooks, `cp_read`/`cp_write`).
   - A kept namespace-scope variable keeps its initializer's dependencies
     (`cp::in`, `cp::out`).
   - Macros from `config.hpp` that are still referenced are kept.
4. **Strip:** removes comments and blank lines and collapses indentation.
5. **Verify:** compiles `submit.cpp` with the profile's compiler and flags.
   If a sample input exists (`--sample=in.txt`), it also runs the program
   built from step 2's output and the one built from step 4's output and
   requires identical output. If anything fails, it falls back to the
   step 2 output and reports which pruning rule needs to be more
   conservative. So tree-shaking can never silently produce a broken
   submission.
6. **Size gate:** fails if the output exceeds 64 KB, listing the largest
   kept declarations.

- Solutions may include `cp/all.hpp`; step 3 removes what isn't used. The
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
  `compat/`. Each entry is a cp name, its std name, the std header it needs,
  and the standard that introduced it. No hand-maintained list.
- **Upgrade:** every reference that the clang AST dump resolves to a cp alias whose
  standard is at or below `--to` is rewritten to the std name
  (`cp::span` → `std::span`). The needed std header is added. Unqualified
  uses through `using namespace` are handled, because the rewrite works on
  resolved references, not text.
- **Downgrade:** every reference to a std facility newer than `--to` that
  has a cp backport is rewritten to the cp name (`std::span` → `cp::span`).
  The corresponding `cp/compat/…` include is added.
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
- **`docs/guide/`**: plain Markdown with a `SUMMARY.md` table of contents
  (the GitBook layout). It renders on GitHub as is and can be published with
  GitBook or mdBook later without changes. Pages:
  1. Getting started: install, template, build modes, `LOCAL`.
  2. Reading and writing: every `read`/`write`/`print` form, options,
     the scoped wrapper, interactive problems, I/O for your own types.
  3. Checks and debugging: `cp::check`, `cp::dbg`, what checked builds catch.
  4. Language levels: what `cp::` aliases exist per standard, `CP_MIN_CXX`,
     each compat name tagged *user* or *internal* (internal names stay public
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

1. `config.hpp`, `compat/` (+ header-isolation and 17/20/23/26 matrix).
2. `sys/` input sources and output sinks.
3. `io/` (reader, writer, codecs, options, format engine) + differential tests + benchmarks.
4. `check.hpp`, `debug.hpp`.
5. xmake build; `xmake bundle` (steps 1–2, 4–6 first, then symbol tree-shaking);
   `dist/cp.hpp`; `xmake migrate`; `xmake prune`.
6. Port existing modules (§12.1).
7. New modules (§12.2).

Each step includes its documentation (§15a); `README.md` is written in step 1
and extended as features land.

## 18. Left to the builder

- Exact internal structure of the format-spec engine and segment table.
- The SWAR digit-validation and combine bit tricks.
- Field limit for the C++17 aggregate codec (suggested 16).
- Benchmark input generation details.
- Internal naming not listed in this document.

## 19. Open items for the owner

- Name of the scoped options wrapper (working name `cp::scoped_options`).
