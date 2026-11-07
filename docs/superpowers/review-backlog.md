# Review backlog

Gaps found during development, parked for a later review pass. Each line names the file, the gap, and the fix.

## MSVC / clang-cl (parked 2026-11-05: MSVC is not a priority)

None of these affect GCC or Clang, including MinGW GCC on Windows (Codeforces).

- `detail/double_int.hpp` `mul_full`: 64-bit product falls back to schoolbook; spec §3 asks for `_umul128` (x64) / `__umulh` (ARM64).
- `detail/double_int.hpp` `div_2by1`: could use `_udiv128` (x64) for `H = u64`.
- `detail/double_int.hpp`, `wide.hpp`: 64-bit add/sub carry could use `_addcarry_u64` / `_subborrow_u64` (x64).
- `compat/bit.hpp` `byteswap`: MSVC uses the portable loop; `_byteswap_ushort/_ulong/_uint64` exist.
- `compat/bit.hpp` `popcount`: `__popcnt64` does not exist on ARM64 MSVC (use `_CountOneBits64`): compile error there.
- `sys/cpu.hpp` `prefetch`: no-op on ARM64 MSVC; `__prefetch(p)` exists.
- `config.hpp` `CP_HAS_SSE42`: MSVC never defines `__SSE4_2__`; it is implied by `/arch:AVX` and above (`__AVX__`).
- clang-cl: `__int128` division and `__builtin_cpu_supports` need compiler-rt, which MSVC's linker does not pull in (link errors). Needs a `CP_MSVC_ABI` flag: `CP_HAS_INT128 0` and the `cpuid` path (with `CP_TARGET("xsave")` for `_xgetbv`).
- `tests/test_cpu.cpp`: includes `<cpuid.h>` on any x64; guard with `__has_include`.

## Windows (MinGW)

- **`sys/os.hpp` (owner, 2026-11-07):** `input_source.hpp` includes `output_sink.hpp` only for `os_chunk_limit` (the
  1 GiB cap on one `ReadFile`/`WriteFile`, Windows only), a sideways dependency between two parallel components, and
  both repeat the Windows include block (`NOMINMAX`, `WIN32_LEAN_AND_MEAN`, `<windows.h>`, `<io.h>`). Fix: an internal
  `cp/sys/os.hpp` (cp::detail) with the platform headers once and `os_chunk_limit` under `CP_OS_WINDOWS` only; source
  and sink both include it, neither includes the other. Check whether `sys/cpu.hpp` belongs on it too.
- **Run under wine (2026-11-06):** MinGW-w64 GCC 16.2 and wine are installed here; `xmake matrix` has `mingw` cells that build every test as a static Windows program (Catch2 from xrepo, no sanitizers on MinGW) and run them with wine. The Win32 reader/writer paths (`ReadFile`/`WriteFile`, `_get_osfhandle`, file mapping) pass `test_io_source`/`test_io_stream`. `xmake bundle --profile=mingw --sample=in.txt` runs both builds with wine.
- Child-process tests run on Windows too: `spawn` reruns the current test case with `CP_TEST_CHILD=k` (no fork there), and `deadline` replaces `alarm`.

## From Plan 3 (debug), 2026-11-06

- **Retargeting the standard streams** (not decided): `io::in.open(path)`, `io::out.open(path)`, `io::err.open(path)`
  so file-I/O judges and log files work through the free functions, `CP_CHECK` and `CP_DBG`. Today they are fixed
  to fds 0/1/2 (a shell or `freopen` redirect still works).
- **Release behaviour of a violated contract** is a no-op (owner's choice). `[[assume]]`/`unreachable` in release was
  considered and not taken.
- ~~`CP_DBG` and `io::err` options~~: obsolete since 2026-11-07, `CP_DBG` is replaced by `io::dump` and streams hold
  no options.
- **Simplification pass** suggested by the owner's question (static_vector's full inplace_vector interface,
  options builders, C++17 ordering backport): owner said "no need" for now.

## From Plan 4b (I/O formatters), 2026-11-07

Deferred minors from the final review, and open questions:

- `io/options.hpp`: the file comment says formatters are "safe to keep"; one built from a temporary `std::string`
  (`range_sep(std::string(...))`) holds a dangling `string_view` (use-after-free, ASan-confirmed). Say "safe to keep
  when built from literals or strings that outlive them".
- `io/writer.hpp` file comment: still says struct members use the argument separator and describes the old two-level
  layout; they use `field_sep`, and layout follows nesting levels.
- `wrap`: measures each value alone, ignoring the current column (the `[file:line]` prefix of `io::dump`, earlier values,
  `name: ` prefixes). Without brackets a broken range starts with a bare newline after a trailing `arg_sep`.
- `writer`/`reader` `m_active` is not restored if a `cp_write`/`cp_read` hook throws: an RAII guard fixes it.
- Short formatter names (`base`, `width`, `precision`, `fill`, `wrap`, `quote`) collide with user globals when
  `cp::io` is opened (`const long long base = 131;` makes `base` ambiguous). Worth a caveat in getting-started.
- **Open (owner to decide):** C++26 structured binding packs (`auto& [...fields] = t;`, `__cpp_structured_bindings >=
  202411L`, GCC 16 and Clang 23) for aggregates with no field limit and no brace-probing caveats; and whether
  `is_tuple_like_v` / `is_plain_aggregate_v` move from `io/codec.hpp` to `cp::traits`.
- **Paste-ready template** (`dist/template.cpp`): deferred by the owner, see spec §19.
