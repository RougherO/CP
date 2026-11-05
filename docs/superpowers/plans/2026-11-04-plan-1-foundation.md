# Plan 1 — Foundation: config + compat layer + intrinsics — Implementation Plan

> **For agentic workers:** executed natively (superpowers:executing-plans).

**Goal:** Header-only `cp` skeleton: `config.hpp`, the post-C++17 compatibility layer, and the intrinsics/CPU layer, compiling on GCC and Clang at C++17/20/23/26.

**Architecture:** Headers under `include/cp/`. `config.hpp` is the only file defining macros. Each `compat/` header mirrors one std header: a facility is a using-declaration of `std::x` when its `__cpp_lib_*` macro is defined, else a cp backport with the identical interface, tagged `// cp:since c++NN`. Larger backports live in `cp::detail::*_backport` so tests can exercise them even when the std alias is active.

**Owner decisions for this plan (2026-11-04):**
- Modules first. Tests stay small and cover only behaviour that is easy to get wrong; no test-first steps, no test per trivial function.
- xmake build and Catch2 tests were pulled forward by the owner: `xmake f --tests=y && xmake test`, and `xmake matrix` runs gcc/clang × 17/20/23/26 (debug: ASan/UBSan, LOCAL). Plan 4 keeps bundle/migrate/prune.
- Docs (README, guide) are deferred to after the module plans.

**Spec:** `docs/superpowers/specs/2026-11-04-cp-library-design.md` (§3–7, §9).

**Later plans:** 2 I/O (§8, new `main.cpp` template, `.vscode`), 3 checks/debug (§10), 4 tooling: xmake build + bundle/migrate/prune (§13, §14), 5 port existing modules (§12.1), 6+ new modules (§12.2), docs (§15a; must carry the user-facing caveats collected in header `@par Caveats` blocks, e.g. source_location column on GCC C++17, and tag each compat name *user* or *internal*).

## Global Constraints

- Compiles at `-std=c++17/20/23/26` with identical behaviour; GCC ≥ 11, Clang ≥ 14; MSVC ≥ 19.30 best-effort (code paths written, not tested).
- 64-bit only: `static_assert(sizeof(void*) == 8)`; unknown architecture is `#error`.
- `config.hpp` is the only header defining macros; all macros `CP_`-prefixed; detection macros always `0`/`1`.
- `cp::` re-exports only facilities introduced after C++17; detection via `__cpp_lib_*` / `__cpp_*`, never `CP_CXXnn`.
- Every versioned `#if` line carries `// cp:since c++20|23|26`.
- Headers: `#pragma once`, self-contained, specific std includes, non-template functions `inline`, no namespace-scope `using namespace`.
- Intrinsics wrapped in `constexpr` functions; non-constant-evaluable ones branch on `cp::is_constant_evaluated()`.
- Do not modify `main.cpp` or `.vscode/`.

## What gets tested (and nothing else)

1. `countl_zero/countr_zero` at 0 and on narrow types; portable fallbacks vs intrinsics.
2. Mixed-sign extremes: `cmp_less(-1, 0u)`, `in_range`, `midpoint(INT_MIN, INT_MAX)`, `saturating_div(INT_MIN, -1)` — as `static_assert`s.
3. Lifetime in `inplace_vector`: every constructed element destroyed exactly once.
4. `mul128`/overflow helpers vs `__int128`.
5. `using namespace std; using namespace cp;` together compiles for every compat name.

## Files

```
include/cp/config.hpp
include/cp/detail/precondition.hpp        temporary; replaced by cp::check in Plan 3
include/cp/compat/{version,meta,source_location,bit,utility,numeric,iterator,span,ranges,inplace_vector}.hpp
include/cp/intrin/{mul,hint}.hpp
include/cp/sys/cpu.hpp
include/cp/all.hpp
xmake.lua, tools/xmake/matrix.lua          build, tests, matrix
tests/lifetime.hpp                        `tracked` counter type
tests/test_{bit,arith,inplace_vector,intrin,both_namespaces}.cpp
```

---

### Task 1: config + base compat (`config`, `version`, `meta`, `source_location`, `precondition`)

- `config.hpp`: `CP_CXX` (uses `_MSVC_LANG`), `CP_CXX17/20/23/26`, `CP_MIN_CXX` (default `201703L`, `#error "cp: language level below CP_MIN_CXX"`), `CP_CLANG/GCC/MSVC` (Clang first), `CP_OS_LINUX/WINDOWS/MACOS/POSIX`, `CP_ARCH_X64/ARM64`, `CP_HAS_INT128`, `CP_HAS_SSE42/AVX2/NEON`, `CP_CHECKS` (1 if `LOCAL` or `!NDEBUG`, overridable), `CP_INLINE`, `CP_NOINLINE`, `CP_LIKELY/UNLIKELY`, `CP_RESTRICT`, `CP_TARGET(isa)`. (No `CP_CONCEPT`: owner ruling, see spec §6.4.)
- `meta.hpp`: `nonesuch`, `is_detected(_v)`, `detected_t` (cp-native); `remove_cvref(_t)`, `type_identity(_t)`, `is_constant_evaluated()` (c++20).
- `source_location.hpp`: `cp::source_location` backport over `__builtin_LINE/COLUMN/FILE/FUNCTION`.
- `precondition.hpp`: `cp::detail::precondition(bool ok, char const* what, source_location = current())` → `"<file>:<line>: <function>: precondition failed: <what>"` on stderr, `abort()`; no-op when `!CP_CHECKS`.
- Tests: none of their own (covered by the both-namespaces TU and use in later headers).

### Task 2: `bit.hpp`
std `<bit>` interface (`bit_cast`, count*, `popcount`, `rotl/rotr`, `bit_width/floor/ceil`, `has_single_bit`, `endian`, `byteswap` c++23). Unsigned-only `static_assert`. Portable `cp::detail::{countl_zero,countr_zero,popcount,byteswap}_portable`.
Test `test_bit.cpp`: exhaustive u8/u16 + random u32/u64 vs loop reference, incl. zero; pow2 edges; rotations with negative/large shifts.

### Task 3: `utility.hpp`, `numeric.hpp`, `iterator.hpp`
`cmp_*`, `in_range`, `to_underlying` (c++23), `unreachable` (c++23); `midpoint` (int/float/pointer), `saturating_add/sub/mul/div`, `saturating_cast` (final C++26 names; draft names were `add_sat`…) (c++26); `ssize`.
Test `test_arith.cpp`: static_asserts for the extremes listed above.

### Task 4: `span.hpp`, `ranges.hpp`
`cp::detail::span_backport<T, Extent>` (full C++20 interface, deduction guides, preconditions); `cp::span` alias in C++17. `cp::views::iota/reverse` in C++17 (random-access iota, pipe for reverse); `namespace views = std::views` when `__cpp_lib_ranges`.
Test: covered by both-namespaces TU (construction, extent deduction, iota+partition_point, reverse pipe).

### Task 5: ~~`expected.hpp`~~ dropped by owner (no CP use; may return as `detail` if a module needs it).

### Task 6: `inplace_vector.hpp`
`cp::detail::inplace_vector_backport<T,N>`, C++26 interface (incl. `try_*`, `unchecked_*`, `erase/erase_if`); `bad_alloc` when full, `out_of_range` from `at`, precondition on `[]`; trivially copyable for trivial T; empty for N == 0.
Test `test_inplace_vector.cpp`: fill-ctor vs iterator-ctor, full capacity, insert/erase, lifetime with `tracked`; same suite against `std::inplace_vector` when available.

### Task 7: `intrin/mul.hpp`, `intrin/hint.hpp`, `sys/cpu.hpp`
(multiply/overflow helpers replaced by `cp/wide.hpp`, done ahead of Task 7); `cpu_feature`, `cpu_supports`, `prefetch` (owner: `assume` and `cycle_counter` dropped).
Test `test_intrin.cpp`: random mul128 vs `__int128` (intrinsic and portable), overflow edges.

### Task 8: `all.hpp` + both-namespaces TU + matrix run
`tests/test_both_namespaces.cpp` calls every compat name unqualified with both namespaces open. `tests/run.sh` passes on gcc/clang × 17/20/23/26. Commit after the matrix is green.
