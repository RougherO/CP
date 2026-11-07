#pragma once
/// @file
/// @brief cp::io::reader: whitespace-separated input of numbers, text, ranges, tuples and plain structs from a file
/// descriptor, safe for interactive problems.
/// @par Caveats
/// - read(), readln() and getline() return a cp::io::read_result: success, or the cp::io::read_error that stopped
///   them. An error makes later reads return it too, until clear(), like `std::cin`'s failure state.
/// - Running out of input is the normal end of a `while (r.read(x))` loop. A bad token (`invalid`, `out_of_range`)
///   is left unread so token() shows it; cp::io::read reports it at the caller's line under `CP_CHECKS`.
/// - A token is complete only when the whitespace after it (or the end of input) has arrived, so an interactive
///   judge must end each line; it always does.
/// - A token longer than the buffer (64 KiB by default) is cut at the buffer's size; strings are not.
/// - getline() drops a trailing `\r`, so Windows line endings read like Unix ones.
/// - The input is opened at the first read, so `std::freopen` on stdin before it still takes effect.
/// - Settings come from formatters (cp/io/options.hpp) passed in the call: `r.read(cp::io::base(16), h)`.
/// - User types: define `void cp_read(cp::io::reader&, T&)` in T's namespace. Plain structs need nothing.
#include "cp/config.hpp"
#include "cp/io/aggregate.hpp"
#include "cp/io/codec.hpp"
#include "cp/io/options.hpp"
#include "cp/io/parse.hpp"
#include "cp/io/writer.hpp"
#include "cp/util/result.hpp"
#include "cp/sys/input_source.hpp"
#include "cp/traits/traits.hpp"
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace cp::detail {

/// @brief `Check<A>` for a read argument `A`, applied to every value inside a formatted value; formatters pass.
template <template <class> class Check, class A, class U = traits::remove_cvref_t<A>>
struct read_targets : Check<A> { };
template <template <class> class Check, class A, class... L>
struct read_targets<Check, A, io::formatting<L...>> : std::true_type { };
template <template <class> class Check, class A, class F, class... Ts>
struct read_targets<Check, A, io::formatted<F, Ts...>> : std::bool_constant<(read_targets<Check, Ts>::value && ...)> {
};

/// @brief True if a read argument `A` is a variable.
template <class A>
using is_variable = std::is_lvalue_reference<A>;
/// @brief True if a read argument `A` is not const.
template <class A>
using is_mutable = std::negation<std::is_const<std::remove_reference_t<A>>>;

}  // namespace cp::detail

namespace cp::io {

/// @brief Why a read produced no value.
enum class read_error : unsigned char {
    end_of_input,  ///< No token left.
    invalid,       ///< The token is not a value of the requested type (`x` for an integer).
    out_of_range,  ///< The token is a number that does not fit the type (`300` for a `std::uint8_t`).
};

/// @brief What a read returns: a value of `T` (nothing for `void`) or the read_error that prevented it.
template <class T = void>
using read_result = util::result<T, read_error>;

/// @brief Reads one file descriptor (0 for cp::io::in).
class reader {
public:
    /// @brief Reads `fd` through a buffer of `capacity` bytes (at least 16, used only when the input is not a
    /// regular file); flushes `tie` before waiting for input.
    explicit reader(int fd = 0, std::size_t capacity = detail::default_buffer_size, writer* tie = nullptr) noexcept
        : m_fd(fd), m_capacity(std::max(capacity, std::size_t{16})), m_tie(tie) { }

    reader(reader const&)                    = delete;
    auto operator=(reader const&) -> reader& = delete;
    ~reader()                                = default;

    /// @brief Reads each argument in turn; a range reads as many elements as it holds.
    template <class... Args>
    auto read(Args&&... args) -> read_result<> {
        static_assert((detail::read_targets<detail::is_variable, Args>::value && ...),
                      "cp::io::reader::read needs variables, not temporaries");
        static_assert((detail::read_targets<detail::is_mutable, Args>::value && ...),
                      "cp::io::reader::read cannot read into const");
        m_open();
        m_read_seq(m_start(), m_start(), args...);
        return m_status();
    }

    /// @brief read(args...), then drops the rest of the line and its `\n`; readln() just skips to the next line.
    /// Tokens left on the line are dropped silently. After an error nothing more is dropped.
    template <class... Args>
    auto readln(Args&&... args) -> read_result<> {
        auto r = read(std::forward<Args>(args)...);
        if (r) m_skip_line();
        return r;
    }

    /// @brief Reads up to the next `\n` (consumed, not stored; a trailing `\r` is dropped too), like `std::getline`.
    auto getline(std::string& line) -> read_result<> {
        m_open();
        if (m_error) return m_status();
        line.clear();
        bool any = false;
        for (;;) {
            auto const n = static_cast<std::size_t>(m_end - m_cur);
            if (auto const* nl = static_cast<char const*>(std::memchr(m_cur, '\n', n))) {
                line.append(m_cur, nl);
                m_cur = nl + 1;
                break;
            }
            line.append(m_cur, n);
            any   |= n != 0;
            m_cur  = m_end;
            if (m_source_done || !m_refill()) {
                if (!any) m_end_of_input();
                break;
            }
        }
        if (!line.empty() && line.back() == '\r') line.pop_back();
        return m_status();
    }

    /// @brief True when every byte of the input has been read (a read that ran into the end makes it true).
    auto eof() const noexcept -> bool { return m_source_done && m_cur == m_end; }
    /// @brief Clears the error that stops further reads.
    void clear() noexcept { m_error.reset(); }
    /// @brief The unread token: after an `invalid` or `out_of_range` error, the bad one (a view into the buffer, valid
    /// until the next read).
    auto token() const noexcept -> std::string_view {
        return m_cur == nullptr ? std::string_view{}
                                : std::string_view(m_cur, static_cast<std::size_t>(detail::token_end(m_cur) - m_cur));
    }
#if CP_CHECKS
    /// @brief The 1-based input line of the next unread byte. Under `CP_CHECKS` only (counting costs time).
    auto line() const noexcept -> std::size_t {
        return m_lines + static_cast<std::size_t>(std::count(m_line_start, m_cur, '\n')) + 1;
    }
#endif

    /// @brief The writer flushed before waiting for input.
    auto tie() const noexcept -> writer* { return m_tie; }
    /// @brief Sets the writer flushed before waiting for input; returns the previous one.
    auto tie(writer* w) noexcept -> writer* { return std::exchange(m_tie, w); }

private:
    friend struct detail::stream_access;
    static constexpr detail::input_settings defaults{};

    // Where a call starts: the defaults, or inside a cp_read hook the settings of the value being read.
    auto m_start() const noexcept -> detail::input_settings const& {
        return m_active != nullptr ? *m_active : defaults;
    }

    // Reads `args` with `governing` until a formatter argument, which governs the arguments after it as `from` +
    // itself.
    template <class... Args>
    void m_read_seq(detail::input_settings const& from, detail::input_settings const& governing, Args&... args) {
        if constexpr ((detail::is_formatter_v<std::remove_const_t<Args>> || ...)) {
            detail::input_settings local;
            detail::input_settings const* current = &governing;
            (m_read_arg(from, local, current, args), ...);
        } else {
            (m_read_value(args, governing), ...);
        }
    }
    template <class T>
    void m_read_arg(detail::input_settings const& from, detail::input_settings& local,
                    detail::input_settings const*& governing, T& x) {
        if constexpr (detail::is_formatter_v<std::remove_const_t<T>>) {
            local = from;
            x.apply(local);
            governing = &local;
        } else {
            m_read_value(x, *governing);
        }
    }

    void m_open() {
        if (CP_LIKELY(m_source.has_value())) return;
        m_source.emplace(m_fd);
        if (m_source->data() != nullptr) {
            m_cur         = m_source->data();
            m_end         = m_cur + m_source->size();
            m_source_done = true;
        } else {
            m_buf    = detail::make_input_buffer(m_capacity);
            m_buf[0] = '\0';
            m_cur = m_end = m_buf.get();
        }
        m_line_start = m_cur;
    }

    // Keeps the unread bytes, reads what is available after them. False if the buffer is full of unread bytes.
    auto m_refill() -> bool {
        auto const kept = static_cast<std::size_t>(m_end - m_cur);
        if (kept >= m_capacity) return false;
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access): only called after m_open()
        if (m_tie != nullptr && m_source->may_block()) m_tie->flush();
#if CP_CHECKS
        m_lines += static_cast<std::size_t>(std::count(m_line_start, m_cur, '\n'));
#endif
        char* buf = m_buf.get();
        std::memmove(buf, m_cur, kept);
        auto const got =
            m_source->read_some(buf + kept, m_capacity - kept);  // NOLINT(bugprone-unchecked-optional-access)
        buf[kept + got] = '\0';
        m_cur = m_line_start = buf;
        m_end                = buf + kept + got;
        m_source_done        = got == 0;
        return true;
    }

    // Moves past the next `\n`, or to the end of input.
    void m_skip_line() {
        for (;;) {
            auto const n = static_cast<std::size_t>(m_end - m_cur);
            if (auto const* nl = static_cast<char const*>(std::memchr(m_cur, '\n', n))) {
                m_cur = nl + 1;
                return;
            }
            m_cur = m_end;
            if (m_source_done || !m_refill()) return;
        }
    }

    // Moves to the next non-whitespace byte; false at end of input.
    auto m_skip_ws() -> bool {
        for (;;) {
            while (static_cast<unsigned char>(*m_cur) - 1U < 32U) ++m_cur;  // 1–32; the '\0' sentinel stops
            if (m_cur != m_end) {
                if (*m_cur != '\0') return true;
                ++m_cur;  // a NUL byte in the input is whitespace
            } else if (m_source_done || !m_refill()) {
                return false;
            }
        }
    }

    // True if the token at `e` runs to the end of the buffered bytes: more of it may still arrive.
    auto m_runs_to_end(char const* e) const noexcept -> bool { return detail::token_end(e) == m_end; }

    // Parses one token with `parse(char const*) -> parse_result`, reading more input while it may be incomplete.
    template <class Parse>
    auto m_parse_token(Parse parse) -> bool {
        if (!m_skip_ws()) return m_end_of_input();
        for (;;) {
            auto const r = parse(m_cur);
            if (m_source_done || !m_runs_to_end(r.end) || !m_refill()) {
                if (CP_UNLIKELY(r.status != detail::parse_status::ok)) return m_bad_token(r.status);
                m_cur = r.end;
                return true;
            }
        }
    }

    auto m_end_of_input() noexcept -> bool {
        m_error = read_error::end_of_input;
        return false;
    }

    // Leaves the bad token unread, so token() shows it.
    auto m_bad_token(detail::parse_status status) noexcept -> bool {
        m_error = status == detail::parse_status::overflow ? read_error::out_of_range : read_error::invalid;
        return false;
    }

    auto m_status() const noexcept -> read_result<> {
        return m_error ? read_result<>(util::failure(*m_error)) : read_result<>();
    }

    template <class T>
    void m_read_value(T& x, detail::input_settings const& s) {
        constexpr auto kind = detail::io_kind_of<std::remove_const_t<T>, false>();
        static_assert(
            kind != detail::io_kind::none,
            "cp::io::reader cannot read this type: define void cp_read(cp::io::reader&, T&) in its namespace");
        static_assert(kind != detail::io_kind::formatter, "cp::io: a formatter is an argument of read, not a value");
        static_assert(!std::is_const_v<T>, "cp::io::reader cannot read into const (such as the keys of a std::map)");
        if (m_error) return;
        if constexpr (kind == detail::io_kind::formatted) {
            detail::input_settings local = s;
            x.fmt.apply(local);
            std::apply([&](auto&... v) { m_read_seq(local, local, v...); }, x.values);
        } else if constexpr (kind == detail::io_kind::hook) {
            auto const* outer = std::exchange(m_active, &s);
            cp_read(*this, x);
            m_active = outer;
        } else if constexpr (kind == detail::io_kind::boolean) {
            long long v = 0;
            m_read_value(v, s);
            x = v != 0;
        } else if constexpr (kind == detail::io_kind::character) {
            m_read_char(x, s);
        } else if constexpr (kind == detail::io_kind::string) {
            m_read_string(x);
        } else if constexpr (kind == detail::io_kind::integer) {
            int const base = s.base;
            m_parse_token([&](char const* p) { return detail::parse_int(p, x, base); });
        } else if constexpr (kind == detail::io_kind::floating) {
            auto const fmt = s.float_format;
            m_parse_token([&](char const* p) { return detail::parse_float(p, x, fmt); });
        } else if constexpr (kind == detail::io_kind::range) {
            using E = detail::range_value_t<T>;
            for (auto&& e : x) {
                if constexpr (std::is_lvalue_reference_v<decltype(e)>) {
                    m_read_value(e, s);
                } else {  // a proxy, as in std::vector<bool>
                    E v{};
                    m_read_value(v, s);
                    e = v;
                }
            }
        } else if constexpr (kind == detail::io_kind::tuple) {
            detail::for_each_element(x, [&](auto& e) { m_read_value(e, s); });
        } else {
            detail::for_each_field(x, [&](auto& f) { m_read_value(f, s); });
        }
    }

    void m_read_char(char& c, detail::input_settings const& s) {
        if (s.skip_ws) {
            if (!m_skip_ws()) return static_cast<void>(m_end_of_input());
        } else if (m_cur == m_end && (m_source_done || !m_refill() || m_cur == m_end)) {
            return static_cast<void>(m_end_of_input());
        }
        c = *m_cur++;
    }

    void m_read_string(std::string& s) {
        if (!m_skip_ws()) return static_cast<void>(m_end_of_input());
        s.clear();
        for (;;) {
            char const* e = detail::token_end(m_cur);  // m_end at the latest: the '\0' sentinel
            s.append(m_cur, e);
            m_cur = e;
            if (e != m_end || m_source_done || !m_refill()) return;
        }
    }

    int m_fd;
    std::size_t m_capacity;
    writer* m_tie;
    std::optional<detail::input_source> m_source;
    std::unique_ptr<char[]> m_buf;  // NOLINT(modernize-avoid-c-arrays): a byte buffer of run-time size
    char const* m_cur                      = nullptr;
    char const* m_end                      = nullptr;
    char const* m_line_start               = nullptr;  // where m_lines last counted up to
    detail::input_settings const* m_active = nullptr;  // during a cp_read hook: the settings of the value it reads
    bool m_source_done                     = false;    // the source has no more bytes
    std::optional<read_error> m_error;                 // stops further reads until clear()
#if CP_CHECKS
    std::size_t m_lines = 0;  // newlines before m_line_start
#endif
};

}  // namespace cp::io
