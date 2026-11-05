#pragma once
/// @file
/// @brief Platform detection and attribute macros. The only header defining macros;
/// detection macros are always 0 or 1.

// ---- language level -------------------------------------------------------

/// @def CP_CXX
/// @brief Active language level (`201703L`, ...); correct on MSVC too.
#if defined(_MSVC_LANG) && _MSVC_LANG > __cplusplus
#  define CP_CXX _MSVC_LANG
#else
#  define CP_CXX __cplusplus
#endif
/// @brief 1 when compiling as C++17 or later.
#define CP_CXX17 (CP_CXX >= 201703L)
/// @brief 1 when compiling as C++20 or later.
#define CP_CXX20 (CP_CXX >= 202002L)
/// @brief 1 when compiling as C++23 or later.
#define CP_CXX23 (CP_CXX >= 202302L)
/// @brief 1 when compiling as C++26 or later.
#define CP_CXX26 (CP_CXX > 202302L)

/// @def CP_MIN_CXX
/// @brief Lowest supported language level; default `201703L`.
#ifndef CP_MIN_CXX
#  define CP_MIN_CXX 201703L
#endif
static_assert(CP_CXX >= CP_MIN_CXX, "cp: language level below CP_MIN_CXX");

// ---- compiler (Clang first: it also defines __GNUC__ / _MSC_VER) -------------

/// @def CP_CLANG
/// @brief 1 on Clang (including clang-cl), else 0.
/// @def CP_GCC
/// @brief 1 on GCC, else 0.
/// @def CP_MSVC
/// @brief 1 on MSVC (not clang-cl), else 0. Best-effort.
#if defined(__clang__)
#  define CP_CLANG 1
#  define CP_GCC   0
#  define CP_MSVC  0
#elif defined(__GNUC__)
#  define CP_CLANG 0
#  define CP_GCC   1
#  define CP_MSVC  0
#elif defined(_MSC_VER)
#  define CP_CLANG 0
#  define CP_GCC   0
#  define CP_MSVC  1
#else
#  error "cp: unsupported compiler"
#endif

// ---- operating system ------------------------------------------------------

/// @def CP_OS_LINUX
/// @brief 1 when targeting Linux.
#if defined(__linux__)
#  define CP_OS_LINUX 1
#else
#  define CP_OS_LINUX 0
#endif
/// @def CP_OS_WINDOWS
/// @brief 1 when targeting Windows.
#if defined(_WIN32)
#  define CP_OS_WINDOWS 1
#else
#  define CP_OS_WINDOWS 0
#endif
/// @def CP_OS_MACOS
/// @brief 1 when targeting macOS.
#if defined(__APPLE__)
#  define CP_OS_MACOS 1
#else
#  define CP_OS_MACOS 0
#endif
/// @brief 1 on Linux or macOS.
#define CP_OS_POSIX (CP_OS_LINUX || CP_OS_MACOS)

// ---- architecture -----------------------------------------------------------

/// @def CP_ARCH_X64
/// @brief 1 on x86-64.
/// @def CP_ARCH_ARM64
/// @brief 1 on AArch64. Other architectures are an error.
#if defined(__x86_64__) || defined(_M_X64)
#  define CP_ARCH_X64   1
#  define CP_ARCH_ARM64 0
#elif defined(__aarch64__) || defined(_M_ARM64)
#  define CP_ARCH_X64   0
#  define CP_ARCH_ARM64 1
#else
#  error "cp: unsupported architecture (x86-64 and ARM64 only)"
#endif
static_assert(sizeof(void*) == 8, "cp: 64-bit targets only");

/// @def CP_HAS_INT128
/// @brief 1 when `__int128` exists.
#if defined(__SIZEOF_INT128__)
#  define CP_HAS_INT128 1
#else
#  define CP_HAS_INT128 0
#endif

/// @def CP_HAS_SSE42
/// @brief 1 when SSE4.2 is enabled at compile time; use cp::cpu_supports at run time.
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
#if defined(__ARM_NEON) || defined(_M_ARM64)
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
#  define CP_HAS_BUILTIN_COLUMN CP_MSVC
#endif

// ---- checks -----------------------------------------------------------------

/// @def CP_CHECKS
/// @brief 1 when checks are on (`LOCAL` defined or `NDEBUG` not); override with `-DCP_CHECKS=0/1`.
#ifndef CP_CHECKS
#  if defined(LOCAL) || !defined(NDEBUG)
#    define CP_CHECKS 1
#  else
#    define CP_CHECKS 0
#  endif
#endif

// ---- attributes -------------------------------------------------------------

/// @def CP_INLINE
/// @brief Force inlining (implies `inline`).
/// @def CP_NOINLINE
/// @brief Prevent inlining.
/// @def CP_LIKELY
/// @brief Hint: `cond` is usually true.
/// @def CP_UNLIKELY
/// @brief Hint: `cond` is usually false.
/// @def CP_RESTRICT
/// @brief No-aliasing pointer qualifier.
/// @def CP_TARGET
/// @brief `CP_TARGET("avx2")`: compile one function for an ISA; guard calls with cp::cpu_supports.
#if CP_MSVC
#  define CP_INLINE      __forceinline
#  define CP_NOINLINE    __declspec(noinline)
#  define CP_LIKELY(x)   (x)
#  define CP_UNLIKELY(x) (x)
#  define CP_RESTRICT    __restrict
#  define CP_TARGET(isa)
#else
#  define CP_INLINE      __attribute__((always_inline)) inline
#  define CP_NOINLINE    __attribute__((noinline))
#  define CP_LIKELY(x)   __builtin_expect(!!(x), 1)
#  define CP_UNLIKELY(x) __builtin_expect(!!(x), 0)
#  define CP_RESTRICT    __restrict__
#  define CP_TARGET(isa) __attribute__((target(isa)))
#endif
