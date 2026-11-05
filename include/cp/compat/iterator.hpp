#pragma once
/// @file
/// @brief Backport of C++20 `std::ssize`.
#include "cp/compat/version.hpp"
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace cp {

#if defined(__cpp_lib_ssize)  // cp:since c++20
using std::ssize;
#else
/// @brief `c.size()` as a signed type (at least `std::ptrdiff_t`), so `i < ssize(v) - 1` is safe.
template <class C>
constexpr auto ssize(C const& c) {
    using R = std::common_type_t<std::ptrdiff_t, std::make_signed_t<decltype(c.size())> >;
    return static_cast<R>(c.size());
}
/// @brief Length of a built-in array as `std::ptrdiff_t`.
template <class T, std::ptrdiff_t N>
constexpr auto ssize(T const (&)[N]) noexcept {
    return N;
}
#endif

}  // namespace cp
