#pragma once
/// @file
/// @brief Where cp::io::in's bytes come from: the whole input in memory when it is a regular file, else chunked reads
/// that return whatever has arrived (pipes, terminals, interactive judges).
/// @par Caveats
/// - `fread` is never used: it waits for a full buffer, which deadlocks interactive problems.
/// - In-memory input is followed by a zero byte, the sentinel the parsers stop at, so they need no bounds checks. A
///   file whose size is an exact multiple of the page size (the byte after the end is unmapped) is read into a heap
///   buffer instead.
/// - POSIX: `fstat`, `mmap`, `read`. Windows: `GetFileType`, `CreateFileMapping`/`MapViewOfFile`, `ReadFile` on the
///   descriptor's handle, so no CRT text-mode translation applies.
#include "cp/config.hpp"
#include <cstddef>
#include <cstring>
#include <memory>
#if CP_OS_POSIX
#  include <cerrno>
#  include <sys/mman.h>
#  include <sys/stat.h>
#  include <unistd.h>
#elif CP_OS_WINDOWS
#  include "cp/sys/output_sink.hpp"  // os_chunk_limit
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

/// @brief Readable zero bytes guaranteed after in-memory input: the `'\0'` sentinel every parser stops at.
inline constexpr std::size_t input_padding = 1;

/// @brief True if mapping a file of `size` bytes leaves a readable zero byte after it in its last page. False for an
/// exact multiple of the page size: the byte after the end is on an unmapped page.
constexpr auto mapping_has_padding(std::size_t size, std::size_t page) noexcept -> bool {
    return size % page != 0;  // also false for an empty file, which is not mapped
}

/// @brief An uninitialised heap buffer of `size` bytes followed by room for input_padding more.
// NOLINTNEXTLINE(modernize-avoid-c-arrays): a byte buffer of run-time size
inline auto make_input_buffer(std::size_t size) -> std::unique_ptr<char[]> {
#if defined(__cpp_lib_smart_ptr_for_overwrite)  // cp:since c++20
    // NOLINTNEXTLINE(modernize-avoid-c-arrays): a byte buffer of run-time size
    return std::make_unique_for_overwrite<char[]>(size + input_padding);
#else
    // NOLINTNEXTLINE(modernize-avoid-c-arrays, cppcoreguidelines-owning-memory): owned by the unique_ptr
    return std::unique_ptr<char[]>(new char[size + input_padding]);
#endif
}

/// @brief Reads one file descriptor (stdin for cp::io::in). Does not close it.
class input_source {
public:
    /// @brief Inspects `fd` and chooses: map or slurp a regular file, else stream it.
    explicit input_source(int fd) noexcept {
#if CP_OS_POSIX
        m_fd = fd;
        struct stat st{};
        if (::fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) return;
        off_t const offset = ::lseek(fd, 0, SEEK_CUR);
        if (offset < 0 || offset > st.st_size) return;
        auto const size = static_cast<std::size_t>(st.st_size - offset);
        auto const page = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
        if (offset == 0 && mapping_has_padding(size, page)) {
            void* view = ::mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
            if (view != MAP_FAILED) {
                m_adopt_view(view, size);
                return;
            }
        }
        m_slurp(size);
#elif CP_OS_WINDOWS
        m_handle = reinterpret_cast<HANDLE>(::_get_osfhandle(fd));  // NOLINT(performance-no-int-to-ptr)
        if (m_handle == INVALID_HANDLE_VALUE || ::GetFileType(m_handle) != FILE_TYPE_DISK) return;
        LARGE_INTEGER total{}, offset{}, zero{};
        if (::GetFileSizeEx(m_handle, &total) == 0 || ::SetFilePointerEx(m_handle, zero, &offset, FILE_CURRENT) == 0 ||
            offset.QuadPart > total.QuadPart)
            return;
        auto const size = static_cast<std::size_t>(total.QuadPart - offset.QuadPart);
        SYSTEM_INFO info{};
        ::GetSystemInfo(&info);
        if (offset.QuadPart == 0 && mapping_has_padding(size, info.dwPageSize)) {
            if (HANDLE mapping = ::CreateFileMappingW(m_handle, nullptr, PAGE_READONLY, 0, 0, nullptr)) {
                void* view = ::MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
                ::CloseHandle(mapping);  // the view keeps the mapping alive
                if (view != nullptr) {
                    m_adopt_view(view, size);
                    return;
                }
            }
        }
        m_slurp(size);
#else
        static_cast<void>(fd);
#endif
    }

    input_source(input_source const&)                    = delete;
    auto operator=(input_source const&) -> input_source& = delete;

    ~input_source() {
        if (!m_mapped) return;
        void* view = const_cast<char*>(m_data);  // NOLINT(cppcoreguidelines-pro-type-const-cast)
#if CP_OS_POSIX
        ::munmap(view, m_size);
#elif CP_OS_WINDOWS
        ::UnmapViewOfFile(view);
#else
        static_cast<void>(view);
#endif
    }

    /// @brief The whole input followed by input_padding zero bytes, or null if the input is streamed.
    auto data() const noexcept -> char const* { return m_data; }
    /// @brief Bytes in data().
    auto size() const noexcept -> std::size_t { return m_size; }

    /// @brief Streamed input: copies up to `max` bytes that are available now (waiting only if none are) and returns
    /// how many; 0 means end of input. In-memory input returns 0.
    auto read_some(char* dst, std::size_t max) noexcept -> std::size_t {
        return m_data != nullptr ? 0 : m_read_raw(dst, max);
    }

    /// @brief True if read_some can wait for data (pipe, terminal): the reader flushes tied output first.
    auto may_block() const noexcept -> bool { return m_data == nullptr; }

private:
    // One read from the descriptor: what is available now, 0 at end of input or on error.
    auto m_read_raw(char* dst, std::size_t max) const noexcept -> std::size_t {
#if CP_OS_POSIX
        for (;;) {
            auto n = ::read(m_fd, dst, max);
            if (n >= 0) return static_cast<std::size_t>(n);
            if (errno != EINTR) return 0;
        }
#elif CP_OS_WINDOWS
        DWORD got = 0;
        auto want = static_cast<DWORD>(max < os_chunk_limit ? max : os_chunk_limit);
        // A pipe whose writer has closed fails with ERROR_BROKEN_PIPE: that is end of input.
        return ::ReadFile(m_handle, dst, want, &got, nullptr) != 0 ? got : 0;
#else
        static_cast<void>(dst), static_cast<void>(max);
        return 0;
#endif
    }

    void m_adopt_view(void* view, std::size_t size) noexcept {
        m_data   = static_cast<char const*>(view);
        m_size   = size;
        m_mapped = true;
    }

    // Reads the `size` remaining bytes of a regular file into a zero-padded heap buffer.
    void m_slurp(std::size_t size) noexcept {
        m_heap          = make_input_buffer(size);
        std::size_t got = 0;
        while (got < size) {
            auto n = m_read_raw(m_heap.get() + got, size - got);
            if (n == 0) break;
            got += n;
        }
        std::memset(m_heap.get() + got, 0, input_padding);
        m_data = m_heap.get();
        m_size = got;
    }

#if CP_OS_POSIX
    int m_fd = -1;
#elif CP_OS_WINDOWS
    HANDLE m_handle = INVALID_HANDLE_VALUE;
#endif
    char const* m_data = nullptr;
    std::size_t m_size = 0;
    std::unique_ptr<char[]> m_heap;  // NOLINT(modernize-avoid-c-arrays): a padded byte buffer of run-time size
    bool m_mapped = false;
};

}  // namespace cp::detail
