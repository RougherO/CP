#pragma once
/// @file
/// @brief cp::io::in, cp::io::out and cp::io::err, the standard streams, and what cp's free I/O functions use of
/// their internals.
/// @par Caveats
/// - cp::io::out is buffered (64 KiB) and flushed at exit, before cp::io::in waits for input, and before a failure
///   report; cp::io::err writes through on every call, so diagnostics survive crashes.
#include "cp/config.hpp"
#include "cp/io/reader.hpp"
#include "cp/io/writer.hpp"

namespace cp::io {

// NOLINTBEGIN(bugprone-throwing-static-initialization): out of memory at start-up ends the program either way
/// @brief Standard output, buffered (64 KiB).
inline writer out{1};
/// @brief Standard error, written through on every call.
inline writer err{2, 4096, true};
/// @brief Standard input, tied to cp::io::out.
inline reader in{0, detail::default_buffer_size, &out};
// NOLINTEND(bugprone-throwing-static-initialization)

}  // namespace cp::io

namespace cp::detail {

/// @brief The internals of cp::io::reader and cp::io::writer that cp's free functions use.
struct stream_access {
    /// @brief The settings a call on `r` starts from.
    static auto start(io::reader const& r) noexcept -> detail::input_settings const& { return r.m_start(); }
    /// @brief Reads `args` from `r` with `governing`; a formatter argument switches to `from` + itself.
    template <class... Args>
    static auto read(io::reader& r, detail::input_settings const& from, detail::input_settings const& governing,
                     Args&... args) -> io::read_result<> {
        r.m_open();
        r.m_read_seq(from, governing, args...);
        return r.m_status();
    }
    /// @brief Moves `r` past the next `\n`.
    static void skip_line(io::reader& r) { r.m_skip_line(); }

    /// @brief Writes `x` to `w` with `s`, after the argument separator unless `first`.
    template <class T>
    static void write_item(io::writer& w, bool& first, detail::output_settings const& s, T const& x) {
        w.m_write_sep_item(first, s.arg_sep, x, s);
    }
    /// @brief Writes `args` to `w` with `governing`; a formatter argument switches to `from` + itself.
    template <class... Args>
    static void write(io::writer& w, detail::output_settings const& from, detail::output_settings const& governing,
                      bool first, Args const&... args) {
        w.m_write_seq(from, governing, first, args...);
    }
};

}  // namespace cp::detail
