#pragma once
/// @file
/// @brief Where cp::io::out's and cp::io::err's bytes go: unbuffered writes of whole blocks to a file descriptor.
/// @par Caveats
/// - POSIX: `write`. Windows: `WriteFile` on the descriptor's handle, so `\n` is never expanded to `\r\n`.
/// - Write errors (closed pipe, full disk) are ignored: the program's output is lost either way.
#include "cp/config.hpp"
#include <cstddef>
#if CP_OS_POSIX
#  include <cerrno>
#  include <unistd.h>
#elif CP_OS_WINDOWS
#  include <io.h>
#  ifndef NOMINMAX  // else <windows.h> defines `min`/`max` macros that break `std::max(a, b)`
#    define NOMINMAX
#  endif
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#endif

namespace cp::detail {

/// @brief Most bytes passed to one OS read or write call (`ReadFile`/`WriteFile` take a 32-bit count).
inline constexpr std::size_t os_chunk_limit = std::size_t{1} << 30;

/// @brief Writes to one file descriptor (1 for cp::io::out, 2 for cp::io::err). Does not close it.
class output_sink {
public:
    /// @brief Writes to `fd`.
    explicit output_sink(int fd) noexcept {
#if CP_OS_POSIX
        m_fd = fd;
#elif CP_OS_WINDOWS
        m_handle = reinterpret_cast<HANDLE>(::_get_osfhandle(fd));  // NOLINT(performance-no-int-to-ptr)
#else
        static_cast<void>(fd);
#endif
    }

    /// @brief Writes all `n` bytes at `p`, retrying partial writes.
    void write_all(char const* p, std::size_t n) const noexcept {
        while (n > 0) {
#if CP_OS_POSIX
            auto w = ::write(m_fd, p, n);
            if (w < 0 && errno == EINTR) continue;
            if (w <= 0) return;
            auto done = static_cast<std::size_t>(w);
#elif CP_OS_WINDOWS
            DWORD done = 0;
            auto want  = static_cast<DWORD>(n < os_chunk_limit ? n : os_chunk_limit);
            if (::WriteFile(m_handle, p, want, &done, nullptr) == 0 || done == 0) return;
#else
            std::size_t done = n;  // no known OS: output is discarded
#endif
            p += done;
            n -= done;
        }
    }

private:
#if CP_OS_POSIX
    int m_fd = -1;
#elif CP_OS_WINDOWS
    HANDLE m_handle = INVALID_HANDLE_VALUE;
#endif
};

}  // namespace cp::detail
