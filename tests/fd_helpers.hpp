#pragma once
// Descriptor, pipe and child-process helpers for the I/O tests (POSIX and Windows).
#include "cp/sys/output_sink.hpp"
#include <catch2/catch_test_macros.hpp>
#include <csignal>
#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#if CP_OS_POSIX
#  include <sys/wait.h>
#  include <unistd.h>
#else
#  include <catch2/interfaces/catch_interfaces_capture.hpp>
#  include <chrono>
#  include <condition_variable>
#  include <mutex>
#  include <thread>
#  include <cstdlib>
#  include <fcntl.h>
#  include <io.h>
#  include <string_view>
#  include <windows.h>
#endif

namespace test {

// The few descriptor calls the tests need, under their POSIX and Windows names.
#if CP_OS_POSIX
inline auto fd_of(std::FILE* f) -> int { return fileno(f); }
inline auto seek(int fd, long pos) -> long { return static_cast<long>(::lseek(fd, pos, SEEK_SET)); }
inline auto make_pipe(int (&fds)[2]) -> bool { return ::pipe(fds) == 0; }  // NOLINT(modernize-avoid-c-arrays)
inline void close_fd(int fd) { ::close(fd); }
inline auto page_size() -> std::size_t { return static_cast<std::size_t>(::sysconf(_SC_PAGESIZE)); }
#else
inline auto fd_of(std::FILE* f) -> int { return _fileno(f); }
inline auto seek(int fd, long pos) -> long { return ::_lseek(fd, pos, SEEK_SET); }
inline auto make_pipe(int (&fds)[2]) -> bool { return ::_pipe(fds, 1 << 16, _O_BINARY) == 0; }  // NOLINT
inline void close_fd(int fd) { ::_close(fd); }
inline auto page_size() -> std::size_t {
    SYSTEM_INFO info{};
    ::GetSystemInfo(&info);
    return info.dwPageSize;
}
#endif

// A new temporary file, removed when closed. Under Wine (UCRT build), std::tmpfile names files from a per-process
// counter, so test programs run side by side (xmake test) collide and get nullptr; GetTempFileNameA creates a unique
// file atomically on every Windows runtime.
inline auto open_temp() -> std::FILE* {
#if CP_OS_POSIX
    return std::tmpfile();
#else
    char dir[MAX_PATH + 1];   // NOLINT(modernize-avoid-c-arrays): a Win32 path buffer
    char name[MAX_PATH + 1];  // NOLINT(modernize-avoid-c-arrays): a Win32 path buffer
    if (::GetTempPathA(sizeof dir, dir) == 0 || ::GetTempFileNameA(dir, "cpt", 0, name) == 0) return nullptr;
    int const fd = ::_open(name, _O_RDWR | _O_BINARY | _O_TEMPORARY);
    return fd < 0 ? nullptr : ::_fdopen(fd, "w+b");
#endif
}

// A temporary file holding `content`, positioned at the start.
struct temp_file {
    std::FILE* f = open_temp();
    explicit temp_file(std::string const& content) {
        REQUIRE(f != nullptr);
        cp::detail::output_sink(fd_of(f)).write_all(content.data(), content.size());
        REQUIRE(seek(fd_of(f), 0) == 0);
    }
    temp_file(temp_file const&)                    = delete;
    auto operator=(temp_file const&) -> temp_file& = delete;
    ~temp_file() { std::fclose(f); }
    auto fd() const -> int { return fd_of(f); }
};

// Both ends of a pipe, closed on destruction unless closed earlier.
struct test_pipe {
    int fds[2]{-1, -1};  // NOLINT(modernize-avoid-c-arrays): pipe() takes int[2]
    test_pipe() { REQUIRE(make_pipe(fds)); }
    test_pipe(test_pipe const&)                    = delete;
    auto operator=(test_pipe const&) -> test_pipe& = delete;
    ~test_pipe() {
        close_read();
        close_write();
    }
    auto read_end() const -> int { return fds[0]; }
    auto write_end() const -> int { return fds[1]; }
    void write(std::string const& s) const { cp::detail::output_sink(fds[1]).write_all(s.data(), s.size()); }
    void close_read() {
        if (fds[0] >= 0) close_fd(std::exchange(fds[0], -1));
    }
    void close_write() {
        if (fds[1] >= 0) close_fd(std::exchange(fds[1], -1));
    }
};

// Everything readable from `fd` until end of input.
inline auto read_all(int fd) -> std::string {
    std::string out, chunk(4096, '\0');
    for (;;) {
#if CP_OS_POSIX
        auto n = ::read(fd, chunk.data(), chunk.size());
#else
        auto n = ::_read(fd, chunk.data(), static_cast<unsigned>(chunk.size()));
#endif
        if (n <= 0) return out;
        out.append(chunk.data(), static_cast<std::size_t>(n));
    }
}

// Everything in the file from its start.
inline auto contents(temp_file const& file) -> std::string {
    REQUIRE(seek(file.fd(), 0) == 0);
    return read_all(file.fd());
}

// What a finished child did.
struct child_result {
    bool aborted;  // ended by std::abort
    std::string out, err;
};

#if CP_OS_POSIX
// A child process running a test body, with its stdin, stdout and stderr connected to pipes.
struct child {
    pid_t pid = -1;
    test_pipe in, out, err;  // the parent writes to in.write_end() and reads out/err.read_end()
};
#else
// No fork on Windows: the child reruns this test case with CP_TEST_CHILD=k, and its k-th spawn runs the body.
struct child {
    PROCESS_INFORMATION process{};
    test_pipe in, out, err;  // the parent writes to in.write_end() and reads out/err.read_end()
    child()                                = default;
    child(child const&)                    = delete;
    auto operator=(child const&) -> child& = delete;
    ~child() {
        if (process.hProcess != nullptr) ::CloseHandle(process.hProcess);
        if (process.hThread != nullptr) ::CloseHandle(process.hThread);
    }
};
#endif

// Closes the child's ends in the parent, so each side sees end of input when the other closes.
inline void keep_parent_ends(child& c) {
    c.in.close_read();
    c.out.close_write();
    c.err.close_write();
}

#if CP_OS_POSIX
// Starts `body` in a forked child (default SIGABRT handling, not Catch2's); returns when it is running.
template <class F>
auto spawn(F body) -> std::unique_ptr<child> {
    auto c = std::make_unique<child>();
    std::fflush(nullptr);
    c->pid = ::fork();
    REQUIRE(c->pid >= 0);
    if (c->pid == 0) {
        std::signal(SIGABRT, SIG_DFL);
        ::dup2(c->in.read_end(), 0);
        ::dup2(c->out.write_end(), 1);
        ::dup2(c->err.write_end(), 2);
        for (auto* p : {&c->in, &c->out, &c->err}) {  // keep only 0–2: a stray write end would hide end of input
            p->close_read();
            p->close_write();
        }
        body();
        ::_exit(0);
    }
    keep_parent_ends(*c);
    return c;
}

// Waits for the child; whether SIGABRT ended it.
inline auto wait_aborted(child& c) -> bool {
    int status = 0;
    ::waitpid(c.pid, &status, 0);
    return WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT;
}

#else
inline auto handle_of(int fd) -> HANDLE { return reinterpret_cast<HANDLE>(::_get_osfhandle(fd)); }  // NOLINT

// Which spawn of the current test case this process is (0: none, the test program itself).
inline auto child_index() -> int {
    static int const index = [] {
        char const* v = std::getenv("CP_TEST_CHILD");  // NOLINT(concurrency-mt-unsafe): read once at start
        return v != nullptr ? std::atoi(v) : 0;        // NOLINT(cert-err34-c): our own variable
    }();
    return index;
}

// The number of the next spawn in the current test case (1, 2, ...).
inline auto next_spawn() -> int {
    static std::string test_case;
    static int count = 0;
    auto const& name = Catch::getResultCapture().getCurrentTestName();
    if (name != test_case) test_case = name, count = 0;
    return ++count;
}

// `name` as a Catch2 test filter: its special characters escaped.
inline auto catch_filter(std::string const& name) -> std::string {
    std::string f;
    for (char c : name) {
        if (std::string_view("\\,[]*~\"").find(c) != std::string_view::npos) f += '\\';
        f += c;
    }
    return f;
}

template <class F>
auto spawn(F body) -> std::unique_ptr<child> {
    int const k = next_spawn();
    auto c      = std::make_unique<child>();
    if (child_index() == k) {  // this process is that child: run the body here
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
        std::signal(SIGABRT, SIG_DFL);
        body();
        std::_Exit(0);
    }
    for (HANDLE h : {handle_of(c->in.write_end()), handle_of(c->out.read_end()), handle_of(c->err.read_end())}) {
        ::SetHandleInformation(h, HANDLE_FLAG_INHERIT, 0);  // only the child's ends go to the child
    }
    std::string program(MAX_PATH, '\0');
    program.resize(::GetModuleFileNameA(nullptr, program.data(), MAX_PATH));
    std::string command =
        "\"" + program + "\" \"" + catch_filter(Catch::getResultCapture().getCurrentTestName()) + "\" -o NUL";
    STARTUPINFOA start{};
    start.cb         = sizeof(start);
    start.dwFlags    = STARTF_USESTDHANDLES;
    start.hStdInput  = handle_of(c->in.read_end());
    start.hStdOutput = handle_of(c->out.write_end());
    start.hStdError  = handle_of(c->err.write_end());
    std::fflush(nullptr);
    std::string const index = std::to_string(k);
    ::SetEnvironmentVariableA("CP_TEST_CHILD", index.c_str());
    BOOL const ok =
        ::CreateProcessA(nullptr, command.data(), nullptr, nullptr, TRUE, 0, nullptr, nullptr, &start, &c->process);
    ::SetEnvironmentVariableA("CP_TEST_CHILD", child_index() != 0 ? std::to_string(child_index()).c_str() : nullptr);
    REQUIRE(ok != 0);
    keep_parent_ends(*c);
    return c;
}

// Waits for the child; whether std::abort ended it (exit code 3).
inline auto wait_aborted(child& c) -> bool {
    ::WaitForSingleObject(c.process.hProcess, INFINITE);
    DWORD code = 0;
    ::GetExitCodeProcess(c.process.hProcess, &code);
    return code == 3;
}
#endif

// Closes the child's stdin, collects its output and waits for it.
inline auto finish(child& c) -> child_result {
    c.in.close_write();
    child_result r{false, read_all(c.out.read_end()), read_all(c.err.read_end())};
    r.aborted = wait_aborted(c);
    return r;
}

// Ends the program after `seconds`, so a deadlock fails instead of hanging.
class deadline {
public:
    explicit deadline(unsigned seconds) {
#if CP_OS_POSIX
        ::alarm(seconds);
#else
        m_watchdog = std::thread([this, seconds] {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (!m_done.wait_for(lock, std::chrono::seconds(seconds), [this] { return m_finished; })) std::_Exit(124);
        });
#endif
    }
    deadline(deadline const&)                    = delete;
    auto operator=(deadline const&) -> deadline& = delete;
    ~deadline() {
#if CP_OS_POSIX
        ::alarm(0);
#else
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_finished = true;
        }
        m_done.notify_one();
        m_watchdog.join();
#endif
    }

private:
#if CP_OS_WINDOWS
    std::mutex m_mutex;
    std::condition_variable m_done;
    bool m_finished = false;
    std::thread m_watchdog;
#endif
};

// Runs `body` in a child with `input` (under 64 KiB) on its stdin.
template <class F>
auto run_child(std::string const& input, F body) -> child_result {
    auto c = spawn(body);
    c->in.write(input);
    return finish(*c);
}

}  // namespace test
