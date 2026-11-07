#pragma once
/// @file
/// @brief cp::io::writer: buffered output of numbers, text, ranges, tuples and plain structs to a file descriptor.
/// @par Caveats
/// - Output reaches the descriptor on flush(), when the buffer fills, and on destruction; a write-through writer
///   (cp::io::err) flushes after every call. Mixing it with `printf`/`std::cout` on the same descriptor reorders
///   output.
/// - Settings come from formatters (cp/io/options.hpp) passed in the call; nothing carries over to the next call.
/// - Nested ranges put one inner range per line; elements are separated by the range separator, the arguments of
///   one call and the members of tuples and structs by the argument separator. No separator is written at the end,
///   and `writeln` adds `\n`.
/// - User types: define `void cp_write(cp::io::writer&, T const&)` in T's namespace. Plain structs need nothing.
#include "cp/config.hpp"
#include "cp/io/aggregate.hpp"
#include "cp/detail/integer.hpp"
#include "cp/io/codec.hpp"
#include "cp/io/field_names.hpp"
#include "cp/io/format.hpp"
#include "cp/io/options.hpp"
#include "cp/sys/output_sink.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace cp::io {

/// @brief Buffered output to one file descriptor (1 for cp::io::out, 2 for cp::io::err).
class writer {
public:
    /// @brief Writes to `fd` through a buffer of `capacity` bytes (at least 256); `write_through` flushes after
    /// every call.
    explicit writer(int fd = 1, std::size_t capacity = detail::default_buffer_size, bool write_through = false)
        : m_sink(fd),
          m_capacity(std::max(capacity, min_capacity)),
          m_buf(new char[m_capacity]),  // NOLINT(cppcoreguidelines-owning-memory): owned by m_buf
          m_pos(m_buf.get()),
          m_base(m_buf.get()),
          m_write_through(write_through) { }

    writer(writer const&)                    = delete;
    auto operator=(writer const&) -> writer& = delete;
    ~writer() { flush(); }

    /// @brief Writes each argument, separated by the argument separator; a formatter argument governs the
    /// arguments after it.
    template <class... Args>
    auto write(Args const&... args) -> writer& {
        m_write_seq(m_start(), m_start(), true, args...);
        if (m_write_through) flush();
        return *this;
    }
    /// @brief write(args...), then `\n`.
    template <class... Args>
    auto writeln(Args const&... args) -> writer& {
        m_write_seq(m_start(), m_start(), true, args...);
        put('\n');
        if (m_write_through) flush();
        return *this;
    }

    /// @brief Writes one character as is: no options, no separator (for cp_write hooks).
    void put(char c) noexcept {
        if (m_pos == m_buf.get() + m_capacity) flush();
        *m_pos++ = c;
    }
    /// @brief Writes the characters of `s` as they are: no options, no separator (for cp_write hooks).
    void put(std::string_view s) noexcept {
        if (s.size() > m_free_space()) {
            flush();
            if (s.size() >= m_usable_capacity()) return m_send(s.data(), s.size());
        }
        std::memcpy(m_pos, s.data(), s.size());
        m_pos += s.size();
    }

    /// @brief Sends the buffered bytes to the descriptor.
    void flush() noexcept {
        m_send(m_base, static_cast<std::size_t>(m_pos - m_base));
        m_pos = m_base;
    }

private:
    friend struct detail::stream_access;
    static constexpr std::size_t min_capacity = 256;
    static constexpr detail::output_settings defaults{};

    // Where a call starts: the defaults, or inside a cp_write hook the settings of the value being written.
    auto m_start() const noexcept -> detail::output_settings const& {
        return m_active != nullptr ? *m_active : defaults;
    }

    // Writes `args` with `governing` until a formatter argument, which governs the arguments after it as `from` +
    // itself. `first`: no separator before the first value.
    template <class... Args>
    void m_write_seq(detail::output_settings const& from, detail::output_settings const& governing,
                     [[maybe_unused]] bool first, Args const&... args) {
        if constexpr ((detail::is_formatter_v<Args> || ...)) {
            detail::output_settings local;
            detail::output_settings const* current = &governing;
            (m_write_arg(from, local, current, first, args), ...);
        } else {
            (m_write_sep_item(first, governing.arg_sep, args, governing), ...);
        }
    }
    template <class T>
    void m_write_arg(detail::output_settings const& from, detail::output_settings& local,
                     detail::output_settings const*& governing, bool& first, T const& x) {
        if constexpr (detail::is_formatter_v<T>) {
            local = from;
            x.apply(local);
            governing = &local;
        } else {
            m_write_sep_item(first, governing->arg_sep, x, *governing);
        }
    }

    auto m_free_space() const noexcept -> std::size_t {
        return m_capacity - static_cast<std::size_t>(m_pos - m_buf.get());
    }
    // The buffer's size from where flush() empties it to: at least min_capacity.
    auto m_usable_capacity() const noexcept -> std::size_t {
        return m_capacity - static_cast<std::size_t>(m_base - m_buf.get());
    }
    // Where flushed bytes go: the descriptor, or the capture string while a value is measured (see m_flat()).
    void m_send(char const* p, std::size_t n) noexcept {
        if (CP_UNLIKELY(m_capture != nullptr)) {
            m_capture->append(p, n);
        } else {
            m_sink.write_all(p, n);
        }
    }
    // At least `n` (<= m_capacity) contiguous bytes at the returned position.
    auto m_reserve(std::size_t n) noexcept -> char* {
        if (m_free_space() < n) flush();
        return m_pos;
    }
    void m_put_n(char c, std::size_t n) noexcept {
        while (n-- > 0) put(c);
    }

    template <class T>
    void m_write_value(T const& x, detail::output_settings const& s) {
        constexpr auto kind = detail::io_kind_of<T, true>();
        static_assert(kind != detail::io_kind::none,
                      "cp::io::writer cannot write this type: define "
                      "void cp_write(cp::io::writer&, T const&) in its namespace");
        static_assert(kind != detail::io_kind::formatter, "cp::io: a formatter is an argument of write, not a value");
        if constexpr (kind == detail::io_kind::formatted) {
            detail::output_settings local = s;
            x.fmt.apply(local);
            std::apply([&](auto const&... v) { m_write_seq(local, local, true, v...); }, x.values);
        } else if constexpr (kind == detail::io_kind::hook) {
            auto const* outer = std::exchange(m_active, &s);
            cp_write(*this, x);
            m_active = outer;
        } else if constexpr (kind == detail::io_kind::boolean) {
            if (s.bool_alpha) {
                put(x ? std::string_view("true") : std::string_view("false"));
            } else {
                put(x ? '1' : '0');
            }
        } else if constexpr (kind == detail::io_kind::character) {
            if (s.quote) {
                m_put_quoted(std::string_view(&x, 1), '\'');
            } else {
                put(x);
            }
        } else if constexpr (kind == detail::io_kind::string) {
            if (s.quote) {
                m_put_quoted(std::string_view(x), '"');
            } else {
                put(std::string_view(x));
            }
        } else if constexpr (kind == detail::io_kind::integer) {
            m_write_integer(x, s);
        } else if constexpr (kind == detail::io_kind::floating) {
            m_write_floating(x, s);
        } else if constexpr (kind == detail::io_kind::range) {
            using E                     = detail::range_value_t<T const>;
            constexpr std::size_t level = detail::nesting_level_v<T>;
            m_write_composite(x, s, s.range_open, s.range_close, s.range_sep[std::min(level, s.range_sep_count) - 1],
                              [&](auto&& item) {
                                  for (auto const& e : x) item([&] { m_write_value(static_cast<E const&>(e), s); });
                              });
        } else if constexpr (kind == detail::io_kind::tuple) {
            m_write_composite(x, s, s.field_open, s.field_close, s.field_sep, [&](auto&& item) {
                detail::for_each_element(x, [&](auto const& e) { item([&] { m_write_value(e, s); }); });
            });
        } else {
            m_write_composite(x, s, s.field_open, s.field_close, s.field_sep, [&](auto&& item) {
                std::size_t i = 0;
                detail::for_each_field(x, [&](auto const& f) {
                    item([&] {
                        if constexpr (detail::has_field_names_v<T>) {
                            if (s.field_names) {
                                put(detail::field_names_v<T>[i]);
                                put(": ");
                            }
                        }
                        m_write_value(f, s);
                    });
                    ++i;
                });
            });
        }
    }

    // A range or record: `open`, the elements `each` yields separated by `sep`, `close`. Under wrap, one that does
    // not fit on its line goes one element per line, indented two more spaces, `sep` without its trailing spaces.
    template <class T, class Each>
    void m_write_composite(T const& x, detail::output_settings const& s, std::string_view open, std::string_view close,
                           std::string_view sep, Each const& each) {
        bool broken = false;
        if (CP_UNLIKELY(s.wrap > 0)) {
            std::string const line = m_flat(x, s);
            if (m_indent + line.size() <= static_cast<std::size_t>(s.wrap)) return put(line);
            broken = true;
            while (!sep.empty() && sep.back() == ' ') sep.remove_suffix(1);
        }
        m_put_bracket(open);
        if (broken) m_indent += 2;
        bool first = true;
        each([&](auto&& write_one) {
            if (!std::exchange(first, false)) m_put_bracket(sep);
            if (broken) m_new_line();
            write_one();
        });
        if (broken) {
            m_indent -= 2;
            m_new_line();
        }
        m_put_bracket(close);
    }

    // `\n` and the current indentation.
    void m_new_line() noexcept {
        put('\n');
        m_put_n(' ', m_indent);
    }

    // `x` written on one line (wrap off), captured instead of sent.
    template <class T>
    auto m_flat(T const& x, detail::output_settings const& s) -> std::string {
        if (m_free_space() < min_capacity) flush();  // the capture needs min_capacity bytes of buffer
        std::string line;
        auto* const outer_capture        = std::exchange(m_capture, &line);
        char* const outer_base           = std::exchange(m_base, m_pos);
        detail::output_settings one_line = s;
        one_line.wrap                    = 0;
        m_write_value(x, one_line);
        flush();
        m_base    = outer_base;
        m_capture = outer_capture;
        return line;
    }

    // `text` between two `q`s, with `q`, `\\`, newlines and tabs escaped.
    void m_put_quoted(std::string_view text, char q) noexcept {
        put(q);
        for (char c : text) {
            if (c == q || c == '\\') {
                put('\\');
                put(c);
            } else if (c == '\n') {
                put("\\n");
            } else if (c == '\t') {
                put("\\t");
            } else {
                put(c);
            }
        }
        put(q);
    }

    // A bracket; none by default, so the common case costs one test.
    void m_put_bracket(std::string_view b) noexcept {
        if (!b.empty()) put(b);
    }
    // One value of a separated sequence: `sep` before every value but the first.
    template <class T>
    void m_write_sep_item(bool& first, std::string_view sep, T const& v, detail::output_settings const& s) {
        if (!std::exchange(first, false)) put(sep);
        m_write_value(v, s);
    }

    template <class T>
    void m_write_integer(T v, detail::output_settings const& o) {
        // The fast path needs the whole number in the buffer: true up to ~800-bit integers (min_capacity bytes).
        if (CP_LIKELY(o.base == 10 && o.width == 0 && !o.show_plus && detail::max_decimal_chars_v<T> <= min_capacity)) {
            m_pos = detail::format_int(m_reserve(detail::max_decimal_chars_v<T>), v);
            return;
        }
        bool const negative = v < 0;
        char const sign     = negative ? '-' : o.show_plus ? '+' : '\0';
        std::array<char, static_cast<std::size_t>(detail::max_int_chars_v<T>)> tmp{};
        char const* digits = tmp.data();
        char const* end    = detail::format_unsigned(tmp.data(), detail::unsigned_abs(v), o.base);
        auto const len     = static_cast<std::size_t>(end - digits) + (sign != '\0' ? 1 : 0);
        auto const width   = static_cast<std::size_t>(std::max(o.width, 0));
        auto const pad     = width > len ? width - len : 0;
        if (o.fill != '0') m_put_n(o.fill, pad);
        if (sign != '\0') put(sign);
        if (o.fill == '0') m_put_n('0', pad);
        put(std::string_view(digits, static_cast<std::size_t>(end - digits)));
    }

    template <class F>
    void m_write_floating(F v, detail::output_settings const& o) {
        auto const bound = static_cast<std::size_t>(detail::max_float_chars<F>(o.float_format, o.precision));
        if (bound <= m_usable_capacity()) {
            char* p = m_reserve(bound);
            m_pos   = detail::format_float(p, p + bound, v, o.float_format, o.precision);
        } else {
            std::string tmp(bound, '\0');
            char* end = detail::format_float(tmp.data(), tmp.data() + bound, v, o.float_format, o.precision);
            put(std::string_view(tmp.data(), static_cast<std::size_t>(end - tmp.data())));
        }
    }

    detail::output_sink m_sink;
    std::size_t m_capacity;
    std::unique_ptr<char[]> m_buf;  // NOLINT(modernize-avoid-c-arrays): a byte buffer of run-time size
    char* m_pos;
    char* m_base;  // where flush() empties the buffer to: its start, unless capturing
    std::string* m_capture                  = nullptr;  // while m_flat() measures a value: where flushed bytes go
    std::size_t m_indent                    = 0;        // wrap's current indentation
    detail::output_settings const* m_active = nullptr;  // during a cp_write hook: the settings of the value it writes
    bool m_write_through;
};

}  // namespace cp::io
