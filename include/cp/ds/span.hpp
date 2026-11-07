#pragma once
/// @file
/// @brief cp::ds::span: `std::span` from C++20; before, a backport with a dynamic extent only.
/// @par Caveats
/// - C++17: no static extents (`span<T, 3>`), `as_bytes` or `size_bytes`; `iterator` is a raw pointer.
/// - C++17: an empty span built from non-pointer iterators has `data() == nullptr`.
#include "cp/config.hpp"
#if defined(__cpp_lib_span)  // cp:since c++20
#  include <span>
#else
#  include "cp/detail/precondition.hpp"
#  include <cstddef>
#  include <iterator>
#  include <limits>
#  include <memory>
#  include <type_traits>
#endif

namespace cp::ds {

#if defined(__cpp_lib_span)  // cp:since c++20
using std::as_bytes;
using std::as_writable_bytes;
using std::dynamic_extent;
using std::span;
#else
/// @brief Extent of a span whose size is only known at run time: the only extent before C++20.
inline constexpr std::size_t dynamic_extent = std::numeric_limits<std::size_t>::max();

template <class T, std::size_t Extent = dynamic_extent>
class span;

}  // namespace cp::ds

namespace cp::detail {

/// @brief Pointer to the element `it` refers to: `it` itself for a pointer; null for an empty non-pointer range.
template <class It>
constexpr auto to_pointer(It it, bool empty) noexcept {
    if constexpr (std::is_pointer_v<It>) {
        return it;
    } else {
        return empty ? nullptr : std::addressof(*it);
    }
}
/// @brief True if a `U` element converts to a `T` element by adding qualifiers only (`int` → `int const`).
template <class U, class T>
inline constexpr bool is_qualification_convertible_v = std::is_convertible_v<U (*)[], T (*)[]>;
/// @brief A contiguous sized range whose elements convert to `T`: `std::data` and `std::size` work on it.
template <class R, class T, class = void>
inline constexpr bool is_span_source_v = false;
/// @cond
template <class R, class T>
inline constexpr bool is_span_source_v<
    R, T, std::void_t<decltype(std::data(std::declval<R&>())), decltype(std::size(std::declval<R&>()))>> =
    is_qualification_convertible_v<std::remove_pointer_t<decltype(std::data(std::declval<R&>()))>, T> &&
    (std::is_lvalue_reference_v<R> || std::is_const_v<T>);  // rvalues only for spans of const
/// @endcond

}  // namespace cp::detail

namespace cp::ds {

/// @brief A view of `size()` contiguous `T`s: C++20's `std::span` with a dynamic extent.
template <class T, std::size_t Extent>
class span {
    static_assert(Extent == dynamic_extent, "cp::ds::span: a static extent needs C++20 (std::span)");

public:
    using element_type                  = T;                                ///< element type
    using value_type                    = std::remove_cv_t<T>;              ///< element type without cv
    using size_type                     = std::size_t;                      ///< size type
    using difference_type               = std::ptrdiff_t;                   ///< distance type
    using pointer                       = T*;                               ///< pointer to an element
    using const_pointer                 = T const*;                         ///< pointer to a const element
    using reference                     = T&;                               ///< element reference
    using const_reference               = T const&;                         ///< const element reference
    using iterator                      = T*;                               ///< iterator
    using reverse_iterator              = std::reverse_iterator<iterator>;  ///< reverse iterator
    static constexpr std::size_t extent = dynamic_extent;                   ///< always dynamic_extent

    /// @brief Empty.
    constexpr span() noexcept = default;
    /// @brief The `count` elements from `first` (a contiguous iterator).
    template <class It, std::enable_if_t<detail::is_qualification_convertible_v<
                                             std::remove_reference_t<decltype(*std::declval<It&>())>, T>,
                                         int> = 0>
    constexpr span(It first, size_type count) : m_data(detail::to_pointer(first, count == 0)), m_size(count) { }
    /// @brief The elements of `[first, last)` (contiguous iterators).
    template <class It, class End,
              std::enable_if_t<!std::is_convertible_v<End, size_type> &&
                                   detail::is_qualification_convertible_v<
                                       std::remove_reference_t<decltype(*std::declval<It&>())>, T>,
                               int> = 0>
    constexpr span(It first, End last)
        : m_data(detail::to_pointer(first, first == last)), m_size(static_cast<size_type>(last - first)) { }
    /// @brief The elements of a C array, `std::array`, `std::vector`, other span or contiguous container.
    template <class R, std::enable_if_t<detail::is_span_source_v<R&&, T>, int> = 0>
    constexpr span(R&& r)  // NOLINT(*-explicit-*,*-forwarding-reference-overload): converts like std::span
        : m_data(std::data(r)), m_size(std::size(r)) { }

    ///@{
    /// @brief The first, last or middle elements, as a span.
    /// @pre They exist.
    constexpr auto first(size_type count) const -> span {
        detail::precondition(count <= size(), "span::first: count > size()");
        return {m_data, count};
    }
    constexpr auto last(size_type count) const -> span {
        detail::precondition(count <= size(), "span::last: count > size()");
        return {m_data + (size() - count), count};
    }
    constexpr auto subspan(size_type offset, size_type count = dynamic_extent) const -> span {
        detail::precondition(offset <= size(), "span::subspan: offset > size()");
        detail::precondition(count == dynamic_extent || count <= size() - offset, "span::subspan: count too big");
        return {m_data + offset, count == dynamic_extent ? size() - offset : count};
    }
    ///@}

    /// @brief Number of elements.
    constexpr auto size() const noexcept -> size_type { return m_size; }
    /// @brief True if there are no elements.
    [[nodiscard]] constexpr auto empty() const noexcept -> bool { return m_size == 0; }
    /// @brief Element `i`.
    /// @pre `i < size()`.
    constexpr auto operator[](size_type i) const -> reference {
        detail::precondition(i < size(), "span: index out of range");
        return m_data[i];
    }
    /// @brief The first element.
    /// @pre Not empty.
    constexpr auto front() const -> reference {
        detail::precondition(!empty(), "span::front on empty span");
        return m_data[0];
    }
    /// @brief The last element.
    /// @pre Not empty.
    constexpr auto back() const -> reference {
        detail::precondition(!empty(), "span::back on empty span");
        return m_data[size() - 1];
    }
    /// @brief Pointer to the first element.
    constexpr auto data() const noexcept -> pointer { return m_data; }

    ///@{
    /// @brief Iterators over the elements, forwards and backwards.
    constexpr auto begin() const noexcept -> iterator { return m_data; }
    constexpr auto end() const noexcept -> iterator { return m_data + size(); }
    constexpr auto rbegin() const noexcept { return reverse_iterator(end()); }
    constexpr auto rend() const noexcept { return reverse_iterator(begin()); }
    ///@}

private:
    pointer m_data   = nullptr;
    size_type m_size = 0;
};

///@{
/// @brief Deduction guides, as for `std::span` (all dynamic before C++20).
template <class It, class EndOrSize>
span(It, EndOrSize) -> span<std::remove_reference_t<decltype(*std::declval<It&>())>>;
template <class R>
span(R&&) -> span<std::remove_pointer_t<decltype(std::data(std::declval<R&>()))>>;
///@}
#endif

}  // namespace cp::ds
