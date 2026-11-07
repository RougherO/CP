#pragma once
/// @file
/// @brief Range and iterator helpers cp's I/O, containers and views share: `std::ranges` from C++20, stand-ins built
/// on `std::begin`/`std::end` and argument-dependent lookup before.
#include "cp/config.hpp"
#include <cstddef>
#include <iterator>
#if defined(__cpp_lib_ranges)  // cp:since c++20
#  include <ranges>
#else
#  include "cp/traits/traits.hpp"
#  include <type_traits>
#  include <utility>
#endif

namespace cp::detail {

#if defined(__cpp_lib_ranges)  // cp:since c++20
/// @brief `std::ranges::input_range<R>`.
template <class R>
inline constexpr bool is_range_v = std::ranges::input_range<R>;
/// @brief `std::ranges::range_value_t<R>`.
template <class R>
using range_value_t = std::ranges::range_value_t<R>;
/// @brief `std::input_iterator<It>`.
template <class It>
inline constexpr bool is_input_iterator_v = std::input_iterator<It>;
/// @brief `std::forward_iterator<It>`: multi-pass, so its distance can be taken first.
template <class It>
inline constexpr bool is_forward_iterator_v = std::forward_iterator<It>;
#else
namespace range_adl {
using std::begin;
using std::end;
/// @brief Type of `begin(r)` with `std::begin` and argument-dependent lookup, as `std::ranges::begin` finds it.
template <class R>
using begin_t = decltype(begin(std::declval<R&>()));
/// @brief Type of `end(r)`, found as for begin_t.
template <class R>
using end_t = decltype(end(std::declval<R&>()));
}  // namespace range_adl
/// @brief True if `R` has `begin`/`end`: members, free functions or a C array (stand-in for
/// `std::ranges::input_range`).
template <class R>
inline constexpr bool is_range_v =
    traits::is_detected_v<range_adl::begin_t, R> && traits::is_detected_v<range_adl::end_t, R>;
/// @brief Element type of range `R` (`bool` for `std::vector<bool>`, not its proxy).
template <class R>
using range_value_t = typename std::iterator_traits<range_adl::begin_t<R>>::value_type;

/// @brief Iterator category of `It`, for detecting iterators.
template <class It>
using iterator_category_t = typename std::iterator_traits<It>::iterator_category;
/// @brief True if `It` is at least an input iterator (stand-in for `std::input_iterator`).
template <class It>
inline constexpr bool is_input_iterator_v =
    std::is_convertible_v<traits::detected_t<iterator_category_t, It>, std::input_iterator_tag>;
/// @brief True if `It` is at least a forward iterator (stand-in for `std::forward_iterator`).
template <class It>
inline constexpr bool is_forward_iterator_v =
    std::is_convertible_v<traits::detected_t<iterator_category_t, It>, std::forward_iterator_tag>;
#endif

/// @brief `std::ranges::begin(r)`, or `begin(r)` with `std::begin` and argument-dependent lookup before C++20.
template <class R>
constexpr auto range_begin(R& r) {
#if defined(__cpp_lib_ranges)  // cp:since c++20
    return std::ranges::begin(r);
#else
    using std::begin;
    return begin(r);
#endif
}
/// @brief `std::ranges::end(r)`, or `end(r)` likewise before C++20.
template <class R>
constexpr auto range_end(R& r) {
#if defined(__cpp_lib_ranges)  // cp:since c++20
    return std::ranges::end(r);
#else
    using std::end;
    return end(r);
#endif
}
/// @brief Number of steps from `first` to `last` (a forward iterator and its sentinel).
template <class It, class Sentinel>
constexpr auto iter_distance(It first, Sentinel last) {
#if defined(__cpp_lib_ranges)  // cp:since c++20
    return static_cast<std::size_t>(std::ranges::distance(first, last));
#else
    return static_cast<std::size_t>(std::distance(first, last));
#endif
}

}  // namespace cp::detail
