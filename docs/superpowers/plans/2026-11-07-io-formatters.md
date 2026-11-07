# Plan 4b — I/O formatters, `readln`, `io::dump` — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Remove stream-held options entirely in favour of per-call composable formatters (`io::writeln(io::precision(3) | io::range_sep(","), x, v)`), give ranges and records a generic per-level layout, add `readln`, and replace `CP_DBG` with the function `io::dump`.

**Architecture:** Each setting is a tiny layer struct that writes one field of a flat `output_settings`/`input_settings`. `formatter<Layers...>` holds the layers (an expression template, flattened by `|`). Streams keep no settings: each `read`/`write` call builds the active settings on the stack from the defaults and the formatter arguments it meets (positional, replacing) or wraps (`f(x)`, layered), and passes them down by `const&`. Layout comes from a compile-time nesting level per type.

**Tech Stack:** C++17–26 header-only, GCC/Clang/MinGW, Catch2, xmake.

**Spec:** `docs/superpowers/specs/2026-11-04-cp-library-design.md` §8.7, §8.8, §8.10, §10 (all amended 2026-11-07).

## Global Constraints

- Every Plan 1/2 constraint holds: C++17 floor, newest std facility first with the C++17 path in `#else`, `// cp:since` tags, self-contained headers, terse one-line `///` docs on public and detail APIs, no helper duplicated across modules.
- No cp free function shares a name and a viable call shape with a std function; `test_both_namespaces.cpp` must call every new public name unqualified with `using namespace std;` open.
- `reader`/`writer` members are the API; free `io::read`/`readln`/`getline`/`write`/`writeln` wrap `io::in`/`io::out` only.
- Nothing persists between calls: no `set`, no `options`, no per-stream setters, no `join`.
- Writing plain values without formatters must not get slower: `xmake bench` and `xmake iobench` numbers before and after go in the ledger (owner rule: benchmark perf-sensitive modules).
- Commit only when the owner asks: no commit steps below.

## Review Focus

1. Scope: in `w.write(f, a, g(b), c)`, `c` gets `f` (not `f | g`), and the next call starts from the defaults; a hook's nested `w.write` inside a formatted value inherits it.
2. `read` with formatter arguments or formatted values (`r.read(io::base(16), h)`, `io::read(io::base(16)(h))`) must pass the `const`/temporary `static_assert`s for the formatter while still rejecting `io::base(16)(5)` (a temporary inside).
3. `std::vector<bool>`, `std::string` inside ranges, and hooked user types (level 0) must keep printing as today under the new level rule.
4. `readln` at end of input (no trailing `\n`) and right after a token that ends the line must not consume the next line.
5. `field_names` on C++17 and on compilers without subobject NTTPs must compile and fall back to positional output, never fail.

---

### Task 1: Per-call formatters; streams lose their settings

**Files:**
- Rewrite: `include/cp/io/options.hpp` (settings structs, layers, `formatter`, `formatted`, setting functions, `with`)
- Modify: `include/cp/io/codec.hpp` (new kinds `io_kind::formatter` and `io_kind::formatted`, checked first)
- Modify: `include/cp/io/writer.hpp`, `include/cp/io/reader.hpp` (no `m_opts`, no `options()`; the active settings are built per call and passed down by `const&`; a private pointer to them lets a hook's nested `w.write` inherit)
- Modify: `include/cp/io/io.hpp` (`detail::read_op` accepts formatter and formatted arguments; value form `read<T...>(fs...)`)
- Test: `tests/test_io_stream.cpp` (replace the two options test cases)

**Interfaces (Produces):**
- `struct output_settings { int base = 10; int width = 0; char fill = ' '; bool show_sign = false; std::chars_format float_format = std::chars_format::fixed; int precision = 9; bool bool_alpha = false; std::string_view arg_sep = " "; ds::span<std::string_view const> range_sep = default_range_sep; std::string_view field_sep = " "; std::string_view range_open, range_close, field_open, field_close; bool quote = false; bool field_names = false; int wrap = 0; };` with `inline constexpr std::array<std::string_view, 2> default_range_sep{" ", "\n"}` (the span points into the formatter, which outlives the call).
- `struct input_settings { int base = 10; std::chars_format float_format = std::chars_format::general; bool skip_ws = true; };`
- `template <class... Layers> struct formatter` with `template <class... Ts> constexpr auto operator()(Ts&&... values) const -> formatted<formatter, Ts&&...>`, `void apply(output_settings&) const`, `void apply(input_settings&) const` (each `static_assert`s every layer supports that direction).
- `template <class... A, class... B> constexpr auto operator|(formatter<A...>, formatter<B...>) -> formatter<A..., B...>`
- `constexpr auto with(Fs... fs)` → `(formatter<>{} | ... | fs)`; `with()` is `formatter<>`, the identity.
- Setting functions in `cp::io`, each returning a one-layer formatter: `base(int)`, `float_format(std::chars_format)`, `precision(int)`, `width(int)`, `fill(char)`, `show_sign(bool = true)`, `bool_alpha(bool = true)`, `skip_ws(bool)`, `arg_sep(std::string_view)`, `field_sep(std::string_view)`, `range_bracket(std::string_view, std::string_view)`, `field_bracket(std::string_view, std::string_view)`, `quote(bool = true)`, `field_names(bool = true)`, `wrap(int)`, `template <class... S> range_sep(S... seps)` (one or more; the layer stores `std::array<std::string_view, sizeof...(S)>`).
- `template <class F, class... Ts> struct formatted { F fmt; std::tuple<Ts...> values; };` (`Ts` are reference types).
- Call semantics (writer and reader alike): a formatter argument sets the governing settings to defaults + that formatter for the arguments after it; a formatted value applies its formatter on top of the governing settings for its own values only; the separator before an argument is the governing `arg_sep`.
- Removed: `output_options`/`input_options` and their per-category structs, `read_options<T>`/`write_options<T>`, every `set_*`, `options()`/`options(o)`.

- [ ] **Step 1: Write the failing tests** in `tests/test_io_stream.cpp`, replacing "writer options" and the reader options cases (keep every expected string they had, rewritten with formatters):

```cpp
TEST_CASE("formatters govern the arguments after them, or the values they wrap", "[io][format]") {
    using namespace cp::io;
    REQUIRE(written([](writer& w) { w.write(width(4), -5, 7); }) == "  -5    7");
    REQUIRE(written([](writer& w) { w.write(width(4)(-5), 7); }) == "  -5 7");
    REQUIRE(written([](writer& w) { w.write(width(4) | fill('0'), -5, 7); }) == "-005 0007");
    REQUIRE(written([](writer& w) { w.write(show_sign(), 7, -7, 0); }) == "+7 -7 +0");
    REQUIRE(written([](writer& w) { w.write(base(16), 255, -255); }) == "ff -ff");
    REQUIRE(written([](writer& w) { w.write(precision(3) | precision(2), 3.14159); }) == "3.14");  // right wins
    REQUIRE(written([](writer& w) { w.write(precision(2), 1.0, with(), 1.0); }) == "1.00 1.000000000");
    REQUIRE(written([](writer& w) { w.write(precision(2), 1.0, bool_alpha(), 1.0, true); }) ==
            "1.00 1.000000000 true");                                       // replace, not layer
    REQUIRE(written([](writer& w) { w.write(with(precision(1), with(bool_alpha())), 0.25, true); }) == "0.2 true");
    REQUIRE(written([](writer& w) { w.write(arg_sep(","), 1, 2, with(), 3); }) == "1,2 3");
    REQUIRE(written([](writer& w) { w.write(precision(1), 1.0, precision(3)(2.0), 3.0); }) == "1.0 2.000 3.0");
    REQUIRE(written([](writer& w) { w.write(precision(2), 1.0).write(1.0); }) == "1.001.000000000");  // per call
}

TEST_CASE("reading through formatters", "[io][format]") {
    using namespace cp::io;
    piped_input p("ff 11 ff", 64);
    int a = 0, b = 0, c = 0;
    REQUIRE(p.in.read(base(16), a, base(2), b));
    REQUIRE((a == 255 && b == 3));
    REQUIRE_FALSE(p.in.read(c));                                           // next call: base 10, "ff" is invalid
    piped_input q("x 7", 64);
    char x = 0, sp = 0;
    int d = 0;
    REQUIRE(q.in.read(x, skip_ws(false)(sp), d));
    REQUIRE((sp == ' ' && d == 7));
}
```

- [ ] **Step 2: Run them to verify they fail**

Run: `xmake f --tests=y && xmake build test_io_stream`
Expected: compile errors (`width`, `with`, `formatted` not declared).

- [ ] **Step 3: Implement** the Produces list. Approach: each layer is a struct with its field(s) and `apply(output_settings&)` and/or `apply(input_settings&)`; `formatter::apply` folds over its tuple of layers. `writer::write(args...)` walks the arguments with a local `output_settings` starting at the defaults; a formatter argument resets it to defaults and applies itself; a formatted value copies it, applies its formatter, writes its values, and drops the copy. Every `write_*` helper takes the settings by `const&`; when no argument is a formatter or formatted value the defaults are a `constexpr` object, so the fast integer path's branches fold away. A private `output_settings const* m_active` is set for the duration of a call so a hook's `w.write(member)` starts from the active settings, not the defaults. The reader mirrors this with `input_settings`. `reader::read`'s `static_assert`s skip formatter arguments and check the references inside formatted values (non-const lvalues).

- [ ] **Step 4: Measure, then run the tests**

Run: `xmake bench --filter=write && xmake iobench` (once on the current tree before Step 3, once after), then `xmake build test_io_stream test_io && xmake run test_io_stream && xmake run test_io`
Expected: tests pass; write throughput equal or better (record both runs in the ledger). Other tests and examples using the old API fail to build until Task 6; build only these two here.

---

### Task 2: Layout by nesting level

**Files:**
- Modify: `include/cp/io/codec.hpp` (add `detail::nesting_level_v<T>`)
- Modify: `include/cp/io/writer.hpp` (range, tuple and aggregate branches)
- Test: `tests/test_io_stream.cpp`

**Interfaces:**
- Consumes: `output_settings::range_sep`, `field_sep`, `range_open/close`, `field_open/close` (Task 1).
- Produces: `template <class T> inline constexpr std::size_t nesting_level_v` — 0 for every kind but range/tuple/aggregate/formatted; a range is 1 + level of its value type; a tuple-like or aggregate is 1 + max over its element/field types; a formatted value is the max over its values. A range at level `L` uses `range_sep[min(L, range_sep.size()) - 1]`.

- [ ] **Step 1: Write the failing tests** (update the "writer formats values, ranges and structs" case: its pair/record line now prints `field_sep` between fields; keep the rest):

```cpp
TEST_CASE("layout follows nesting levels", "[io][format]") {
    using namespace cp::io;
    std::map<int, int> m{{1, 10}, {2, 20}};
    std::map<int, std::vector<int>> mv{{1, {5, 6}}, {2, {7}}};
    std::vector<std::vector<std::vector<int>>> c{{{1, 2}, {3, 4}}, {{5, 6}}};
    REQUIRE(written([&](writer& w) { w.write(m); }) == "1 10\n2 20");
    REQUIRE(written([&](writer& w) { w.write(std::vector<std::pair<int, int>>{{1, 2}, {3, 4}}); }) == "1 2\n3 4");
    REQUIRE(written([&](writer& w) { w.write(mv); }) == "1 5 6\n2 7");
    REQUIRE(written([&](writer& w) { w.write(c); }) == "1 2\n3 4\n5 6");
    REQUIRE(written([&](writer& w) { w.write(range_sep(" ", "\n", "\n\n")(c)); }) == "1 2\n3 4\n\n5 6");
    auto py = range_sep(", ") | range_bracket("[", "]") | field_sep(", ") | field_bracket("(", ")");
    REQUIRE(written([&](writer& w) { w.write(py(m)); }) == "[(1, 10), (2, 20)]");
    REQUIRE(written([&](writer& w) { w.write(py(mv)); }) == "[(1, [5, 6]), (2, [7])]");
    REQUIRE(written([&](writer& w) { w.write(py(std::array<int, 2>{1, 2}), std::vector<bool>{true, false}); }) ==
            "[1, 2] 1 0");
    REQUIRE(written([&](writer& w) { w.write(py(std::vector<std::string>{"a b", "c"})); }) == "[a b, c]");
}
```

- [ ] **Step 2: Run to verify failure**

Run: `xmake build test_io_stream && xmake run test_io_stream "[format]"`
Expected: FAIL on the `m` line (today prints `1 10 2 20`).

- [ ] **Step 3: Implement** `nesting_level_v` and the three writer branches: write `open`, elements separated by the level's separator (ranges) or `field_sep` (records), then `close`. The level is a compile-time constant, so the plain-value path gains only one array index.

- [ ] **Step 4: Run tests and measure**

Run: `xmake build test_io_stream && xmake run test_io_stream && xmake bench --filter=write && xmake iobench`
Expected: tests pass; write throughput within noise of the numbers recorded before Task 1 (record both in the ledger).

---

### Task 3: `quote` and `field_names`

**Files:**
- Create: `include/cp/io/field_names.hpp` (`detail::field_names_v<T>`)
- Modify: `include/cp/io/writer.hpp` (string/char branches, aggregate branch)
- Test: `tests/test_io_stream.cpp`

**Interfaces:**
- Produces: `template <class T> inline constexpr std::array<std::string_view, field_count_v<T>> field_names_v` and `inline constexpr bool has_field_names_v<T>`: reflection (`std::meta::identifier_of`) under `__cpp_impl_reflection`; else under `__cpp_nontype_template_args >= 201911L` the `__PRETTY_FUNCTION__` trick (pointer to each field of a declared-only `extern` object of type `T` as a template argument; the name is the identifier before the closing `)`/`]` of the signature, cut at compile time); else `has_field_names_v` is false.
- `quote`: strings as `"…"`, chars as `'…'`, escaping `"` (strings) or `'` (chars), `\\`, `\n`, `\t`.
- `field_names` on an aggregate with names: `name: value` per field; without names, or on tuples/pairs: positional.

- [ ] **Step 1: Write the failing tests**

```cpp
namespace { struct edge { int u, v; long long w; }; }
TEST_CASE("quote and field names", "[io][format]") {
    using namespace cp::io;
    REQUIRE(written([](writer& w) { w.write(quote()(std::string("a \"b\"\n"), 'c', '\'')); }) ==
            R"("a \"b\"\n" 'c' '\'')");
    auto named = written([](writer& w) { w.write((field_names() | field_bracket("{", "}") | field_sep(", "))(edge{1, 2, 5})); });
#if defined(__cpp_impl_reflection) || (defined(__cpp_nontype_template_args) && __cpp_nontype_template_args >= 201911L)
    REQUIRE(named == "{u: 1, v: 2, w: 5}");
#else
    REQUIRE(named == "{1, 2, 5}");
#endif
    REQUIRE(written([](writer& w) { w.write(field_names()(std::pair{1, 2})); }) == "1 2");
}
```

- [ ] **Step 2: Run to verify failure**

Run: `xmake build test_io_stream && xmake run test_io_stream "[format]"`
Expected: FAIL on the first REQUIRE: the `quote` and `field_names` layers exist since Task 1, but the writer ignores them.

- [ ] **Step 3: Implement** `field_names.hpp` and the two writer branches. Verify the C++20 trick's extraction on both compilers' `__PRETTY_FUNCTION__` spellings with a `static_assert` in the test (`field_names_v<edge>[1] == "v"` under the same `#if`).

- [ ] **Step 4: Run across the matrix**

Run: `xmake matrix`
Expected: all cells build and pass; C++17 cells take the positional branch.

---

### Task 4: `wrap`

**Files:**
- Modify: `include/cp/io/writer.hpp`
- Test: `tests/test_io_stream.cpp`

**Interfaces:**
- Consumes: `output_settings::wrap` (Task 1), layout branches (Task 2).
- Behaviour: when `wrap > 0` and a range or record at the top of a written argument would be longer than `wrap` characters, write its `open`, then each element on its own line indented two spaces per depth, each followed by its separator with trailing spaces removed, then `close` on its own line at the parent's indent; elements that fit stay on one line. Measured by writing the element flat into a scratch `std::string` (only when `wrap > 0`).

- [ ] **Step 1: Write the failing test**

```cpp
TEST_CASE("wrap breaks wide values pprint style", "[io][format]") {
    using namespace cp::io;
    auto py = range_sep(", ") | range_bracket("[", "]");
    std::vector<std::vector<int>> g{{1, 2, 3}, {4, 5, 6}};
    REQUIRE(written([&](writer& w) { w.write((py | wrap(80))(g)); }) == "[[1, 2, 3], [4, 5, 6]]");
    REQUIRE(written([&](writer& w) { w.write((py | wrap(12))(g)); }) == "[\n  [1, 2, 3],\n  [4, 5, 6]\n]");
}
```

- [ ] **Step 2: Run to verify failure** — Run: `xmake build test_io_stream && xmake run test_io_stream "[format]"`. Expected: FAIL on the `wrap(12)` line.
- [ ] **Step 3: Implement** as described.
- [ ] **Step 4: Run** — same command. Expected: PASS.

---

### Task 5: `readln` and stream-less `getline`

**Files:**
- Modify: `include/cp/io/reader.hpp` (`readln`), `include/cp/io/io.hpp` (free `readln`, `getline(std::string&)`; remove `getline(reader&, std::string&)`)
- Test: `tests/test_io_stream.cpp`, `tests/test_io.cpp`

**Interfaces:**
- Produces: `template <class... Args> auto reader::readln(Args&&... args) -> read_result<>` (reads like `read`, then consumes through the next `\n`, or to end of input; leftover tokens dropped silently); free `io::readln(args...)` with the same `CP_CHECKS` bad-token report at the caller's line as `io::read`; free `io::getline(std::string&) -> read_result<>` on `io::in`.

- [ ] **Step 1: Write the failing tests**

```cpp
TEST_CASE("readln drops the rest of the line", "[io][stream]") {
    piped_input p("5 junk\nhello world\n7\n8", 64);
    int n = 0, k = 0, j = 0;
    std::string line;
    REQUIRE(p.in.readln(n));
    REQUIRE(p.in.getline(line));
    REQUIRE((n == 5 && line == "hello world"));
    REQUIRE(p.in.readln(k));                                // the token ends the line: only its '\n' goes
    REQUIRE(p.in.readln(j));                                // no trailing '\n' at end of input
    REQUIRE((k == 7 && j == 8));
    REQUIRE_FALSE(p.in.readln(j));
}
```
In `tests/test_io.cpp`, change the two `cp::io::getline(cp::io::in, s)` calls to `cp::io::getline(s)`.

- [ ] **Step 2: Run to verify failure** — Run: `xmake build test_io_stream`. Expected: compile error (`readln` not a member).
- [ ] **Step 3: Implement** `readln` (after the reads, `memchr` for `\n` across refills, as `getline` does, without storing) and the free functions.
- [ ] **Step 4: Run** — `xmake build test_io_stream test_io && xmake run test_io_stream && xmake run test_io`. Expected: PASS.

---

### Task 6: `io::dump` replaces `CP_DBG`; callers, docs, examples

**Files:**
- Create: `include/cp/io/dump.hpp`; Delete: `include/cp/debug/dbg.hpp`
- Modify: `include/cp/debug/debug.hpp` (drop the dbg include), `include/cp/io/io.hpp` or `include/cp/all.hpp` (include `dump.hpp` where the umbrella should expose it), `include/cp/detail/precondition.hpp` (comment)
- Modify: `tests/test_debug.cpp`, `tests/test_both_namespaces.cpp`
- Modify: `examples/debugging.cpp`, `examples/io_options.cpp`, `examples/io_read.cpp`; then `xmake docs` to sync `docs/guide/{debugging,io}.md`; hand-edit prose in `docs/guide/io.md` (settings table, layout, `readln`), `docs/guide/debugging.md`, `docs/guide/bundling.md:44`, `docs/guide/contributing.md:27`, `README.md:58`

**Interfaces:**
- Produces: `template <class... Rest> void dump(detail::located_arg<detail::dump_op, io::writer&> first, Rest const&... rest)` and `void dump(debug::source_location loc = debug::source_location::current())`; `inline constexpr auto dump_style` = the preset in spec §10 (`arg_sep(" | ") | range_sep(", ") | range_bracket("[", "]") | field_sep(", ") | field_bracket("{", "}") | bool_alpha() | float_format(std::chars_format::general) | precision(-1) | quote() | field_names() | wrap(80)`); formatter arguments to `dump` replace `dump_style` with `dump_style | f`. Under `CP_CHECKS`: flush `io::out`, write `[file:line]` (via `detail::file_line`), then, if there are values, a space and the values written by `io::err` starting from `dump_style` instead of the defaults (a private writer entry point taking the starting settings), then `\n`; otherwise empty bodies.

- [ ] **Step 1: Rewrite the failing tests** in `tests/test_debug.cpp`: the evaluation test becomes "io::dump evaluates its arguments either way" (`REQUIRE(r.out == "1")` after `dump(counted())`, independent of `CP_CHECKS`); the print test expects

```cpp
REQUIRE(r.err == at(0) + " 7\n" + at(1) + " 8 | [1, 2] | \"s\"\n" + at(2) + "\n");
```
for `dump(x); dump(x + 1, v, "s"); dump();` (the `[file:line]` prefix without expression text). In `test_both_namespaces.cpp`, replace the `CP_DBG` line with `dump(precision(3), v, sum)`, the `getline(in, line)` and options lines with `getline(line); readln(n); read(base(16), n); writeln(with(range_sep(","), field_sep(","))(v), width(2) | fill('0') | show_sign() | bool_alpha() | quote() | field_names() | wrap(80) | range_bracket("[", "]") | field_bracket("(", ")") | arg_sep(" ") | float_format(std::chars_format::fixed) | precision(3), 1);`.

- [ ] **Step 2: Run to verify failure** — Run: `xmake build test_debug test_both_namespaces`. Expected: compile error (`dump` not declared).
- [ ] **Step 3: Implement** `dump.hpp`, delete `dbg.hpp`, update the examples to the new API (`io_options.cpp` shows a positional formatter, `with()` resetting, a wrapped value, a `constexpr` preset, and `read(base(16), x)`), run `xmake docs`, edit the guide prose.
- [ ] **Step 4: Full verification**

Run: `xmake build -g tests && xmake test && xmake docs --check && xmake matrix`, then for each `examples/<e>.cpp`: `xmake bundle --std=c++17 --input=examples/<e>.cpp --output=build/check/<e>.cpp`
Expected: every test passes, docs in sync, all 12 matrix cells green, every bundle line ends in `compiles`.
