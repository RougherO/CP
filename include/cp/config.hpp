#pragma once
/// @file
/// @brief Platform detection and attribute macros. The only header defining macros; detection macros are always 0
/// or 1. Supported: GCC and Clang (MinGW-w64 on Windows), 64-bit x86-64 and AArch64, C++17 and later.
#if __has_include(<version>)  // feature-test macros (__cpp_lib_*) for every header
#  include <version>
#endif

static_assert(__cplusplus >= 201703L, "cp: C++17 or later required");
#if !defined(__GNUC__)  // Clang defines it too
#  error "cp: GCC or Clang required"
#endif

// ---- operating system ------------------------------------------------------

/// @def CP_OS_WINDOWS
/// @brief 1 when targeting Windows.
#if defined(_WIN32)
#  define CP_OS_WINDOWS 1
#else
#  define CP_OS_WINDOWS 0
#endif
/// @brief 1 on Linux or macOS.
#if defined(__linux__) || defined(__APPLE__)
#  define CP_OS_POSIX 1
#else
#  define CP_OS_POSIX 0
#endif

// ---- architecture -----------------------------------------------------------

/// @def CP_ARCH_X64
/// @brief 1 on x86-64.
/// @def CP_ARCH_ARM64
/// @brief 1 on AArch64. Other architectures are an error.
#if defined(__x86_64__)
#  define CP_ARCH_X64   1
#  define CP_ARCH_ARM64 0
#elif defined(__aarch64__)
#  define CP_ARCH_X64   0
#  define CP_ARCH_ARM64 1
#else
#  error "cp: unsupported architecture (x86-64 and ARM64 only)"
#endif
#if !defined(__SIZEOF_INT128__)
#  error "cp: needs __int128 (64-bit GCC or Clang)"
#endif

/// @def CP_HAS_SSE42
/// @brief 1 when SSE4.2 is enabled at compile time; use cp::sys::cpu_supports at run time.
#if defined(__SSE4_2__)
#  define CP_HAS_SSE42 1
#else
#  define CP_HAS_SSE42 0
#endif
/// @def CP_HAS_AVX2
/// @brief 1 when AVX2 is enabled at compile time.
#if defined(__AVX2__)
#  define CP_HAS_AVX2 1
#else
#  define CP_HAS_AVX2 0
#endif
/// @def CP_HAS_NEON
/// @brief 1 when NEON is available.
#if defined(__ARM_NEON)
#  define CP_HAS_NEON 1
#else
#  define CP_HAS_NEON 0
#endif
/// @def CP_HAS_BUILTIN_COLUMN
/// @brief 1 when `__builtin_COLUMN()` exists (not on GCC).
#if defined(__has_builtin)
#  if __has_builtin(__builtin_COLUMN)
#    define CP_HAS_BUILTIN_COLUMN 1
#  endif
#endif
#ifndef CP_HAS_BUILTIN_COLUMN
#  define CP_HAS_BUILTIN_COLUMN 0
#endif

// ---- checks -----------------------------------------------------------------

/// @def CP_CHECKS
/// @brief 1 when cp checks preconditions and input (reporting the caller's line), else 0. xmake's debug mode
/// defines it as 1; submissions get 0 unless `xmake bundle --checks=1`.
#ifndef CP_CHECKS
#  define CP_CHECKS 0
#endif

// ---- attributes -------------------------------------------------------------

/// @def CP_NOINLINE
/// @brief Prevent inlining.
#define CP_NOINLINE __attribute__((noinline))
/// @def CP_LIKELY
/// @brief Hint: `cond` is usually true.
#define CP_LIKELY(x) __builtin_expect(!!(x), 1)
/// @def CP_UNLIKELY
/// @brief Hint: `cond` is usually false.
#define CP_UNLIKELY(x) __builtin_expect(!!(x), 0)
/// @def CP_TARGET
/// @brief `CP_TARGET("avx2")`: compile one function for an ISA; guard calls with cp::sys::cpu_supports.
#define CP_TARGET(isa) __attribute__((target(isa)))
