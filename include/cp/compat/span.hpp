#pragma once
/// @file
/// @brief Backport of C++20 `std::span`.
/// @par Caveats
/// - `iterator` is a raw pointer in the backport.
/// - An empty span built from an iterator pair has `data() == nullptr`.
#include "cp/compat/version.hpp"
#if defined(__cpp_lib_span)  // cp:since c++20
#  include <span>
#else
#  include "cp/compat/meta.hpp"
#  include "cp/detail/precondition.hpp"
#  include <array>
#  include <cstddef>
#  include <iterator>
#  include <limits>
#  include <memory>
#  include <type_traits>
#endif

namespace cp {

#if defined(__cpp_lib_span)  // cp:since c++20
using std::as_bytes;
using std::as_writable_bytes;
using std::dynamic_extent;
using std::span;
#else
/// @brief Extent of a span whose size is only known at run time.
inline constexpr std::size_t dynamic_extent = std::numeric_limits<std::size_t>::max();

template <class T, std::size_t Extent = dynamic_extent>
class span;

namespace detail {

/// @brief Size storage: nothing for a static extent, a `size_t` for a dynamic one.
template <std::size_t Extent>
struct span_extent {
    constexpr span_extent(std::size_t) noexcept { }
    static constexpr auto size() noexcept { return Extent; }
};
template <>
struct span_extent<dynamic_extent> {
    constexpr span_extent(std::size_t n) noexcept : m_size(n) { }
    constexpr auto size() const noexcept { return m_size; }

private:
    std::size_t m_size;
};

template <class T>
inline constexpr bool is_span_v = false;
template <class T, std::size_t E>
inline constexpr bool is_span_v<span<T, E> > = true;
template <class T>
inline constexpr bool is_std_array_v = false;
template <class T, std::size_t N>
inline constexpr bool is_std_array_v<std::array<T, N> > = true;

/// @brief `From*` converts to `To*` by qualification only (`int*` → `int const*`, not `Derived*` → `Base*`).
template <class From, class To>
inline constexpr bool is_qualification_convertible_v = std::is_convertible_v<From (*)[], To (*)[]>;

template <class R>
using range_data_t = decltype(std::data(std::declval<R&>()));
template <class R>
using range_size_t = decltype(std::size(std::declval<R&>()));

/// @brief Contiguous sized range usable by the span range constructor (C++17 approximation of the std constraint).
template <class R, class T, class = void>
inline constexpr bool is_span_compatible_range_v = false;
template <class R, class T>
inline constexpr bool is_span_compatible_range_v<R, T, std::void_t<range_data_t<R>, range_size_t<R> > > =
    !is_span_v<remove_cvref_t<R> > && !is_std_array_v<remove_cvref_t<R> > && !std::is_array_v<remove_cvref_t<R> > &&
    is_qualification_convertible_v<std::remove_pointer_t<range_data_t<R> >, T> &&
    (std::is_lvalue_reference_v<R> || std::is_const_v<T>);  // rvalues only for spans of const

/// @brief Iterator usable by the `(first, count)` and `(first, last)` constructors.
template <class It, class T, class = void>
inline constexpr bool is_span_compatible_iterator_v = false;
template <class It, class T>
inline constexpr bool is_span_compatible_iterator_v<It, T, std::void_t<decltype(*std::declval<It&>())> > =
    is_qualification_convertible_v<std::remove_reference_t<decltype(*std::declval<It&>())>, T>;

template <class It>
constexpr auto to_pointer(It it, std::size_t n) noexcept {
    if constexpr (std::is_pointer_v<It>) {
        return it;
    } else {
        using P = decltype(std::addressof(*it));
        return n == 0 ? P{} : std::addressof(*it);
    }
}

}  // namespace detail

/// @brief Non-owning view of a contiguous sequence; static or dynamic extent.
template <class T, std::size_t Extent>
class span : private detail::span_extent<Extent> {
    using extent_base                = detail::span_extent<Extent>;
    static constexpr bool is_dynamic = Extent == dynamic_extent;

public:
    using element_type     = T;
    using value_type       = std::remove_cv_t<T>;
    using size_type        = std::size_t;
    using difference_type  = std::ptrdiff_t;
    using pointer          = T*;
    using const_pointer    = T const*;
    using reference        = T&;
    using const_reference  = T const&;
    using iterator         = T*;
    using reverse_iterator = std::reverse_iterator<iterator>;

    static constexpr std::size_t extent = Extent;

    /// @brief Empty span; only for dynamic or zero extent.
    template <std::size_t E = Extent, std::enable_if_t<E == dynamic_extent || E == 0, int> = 0>
    constexpr span() noexcept : extent_base(0), m_data(nullptr) { }

    /// @brief `[first, first + count)`. Explicit for a static extent.
    template <class It, std::size_t E = Extent,
              std::enable_if_t<E == dynamic_extent && detail::is_span_compatible_iterator_v<It, T>, int> = 0>
    constexpr span(It first, size_type count) : extent_base(count), m_data(detail::to_pointer(first, count)) { }
    template <class It, std::size_t E = Extent,
              std::enable_if_t<E != dynamic_extent && detail::is_span_compatible_iterator_v<It, T>, int> = 0>
    constexpr explicit span(It first, size_type count) : extent_base(count), m_data(detail::to_pointer(first, count)) {
        detail::precondition(count == Extent, "span: count != extent");
    }

    /// @brief `[first, last)`. Explicit for a static extent.
    template <class It, class End, std::size_t E = Extent,
              std::enable_if_t<E == dynamic_extent && detail::is_span_compatible_iterator_v<It, T> &&
                                   !std::is_convertible_v<End, std::size_t>,
                               int> = 0>
    constexpr span(It first, End last) : span(first, static_cast<size_type>(last - first)) { }
    template <class It, class End, std::size_t E = Extent,
              std::enable_if_t<E != dynamic_extent && detail::is_span_compatible_iterator_v<It, T> &&
                                   !std::is_convertible_v<End, std::size_t>,
                               int> = 0>
    constexpr explicit span(It first, End last) : span(first, static_cast<size_type>(last - first)) { }

    /// @brief Whole built-in array.
    template <std::size_t N, std::enable_if_t<Extent == dynamic_extent || Extent == N, int> = 0>
    constexpr span(type_identity_t<element_type> (&arr)[N]) noexcept : extent_base(N), m_data(arr) { }

    /// @brief Whole `std::array`.
    template <class U, std::size_t N,
              std::enable_if_t<
                  (Extent == dynamic_extent || Extent == N) && detail::is_qualification_convertible_v<U, T>, int> = 0>
    constexpr span(std::array<U, N>& arr) noexcept : extent_base(N), m_data(arr.data()) { }
    template <
        class U, std::size_t N,
        std::enable_if_t<
            (Extent == dynamic_extent || Extent == N) && detail::is_qualification_convertible_v<U const, T>, int> = 0>
    constexpr span(std::array<U, N> const& arr) noexcept : extent_base(N), m_data(arr.data()) { }

    /// @brief Contiguous container with `data()` and `size()`. Explicit for a static extent.
    template <class R, std::size_t E = Extent,
              std::enable_if_t<E == dynamic_extent && detail::is_span_compatible_range_v<R&&, T>, int> = 0>
    constexpr span(R&& r) : extent_base(std::size(r)), m_data(std::data(r)) { }
    template <class R, std::size_t E = Extent,
              std::enable_if_t<E != dynamic_extent && detail::is_span_compatible_range_v<R&&, T>, int> = 0>
    constexpr explicit span(R&& r) : extent_base(std::size(r)), m_data(std::data(r)) {
        detail::precondition(std::size(r) == Extent, "span: range size != extent");
    }

    /// @brief Conversion from another span (adding `const`, or static to dynamic extent).
    template <class U, std::size_t N,
              std::enable_if_t<(Extent == dynamic_extent || N == dynamic_extent || Extent == N) &&
                                   detail::is_qualification_convertible_v<U, T> &&
                                   !(Extent != dynamic_extent && N == dynamic_extent),
                               int> = 0>
    constexpr span(span<U, N> const& s) noexcept : extent_base(s.size()), m_data(s.data()) { }
    template <
        class U, std::size_t N,
        std::enable_if_t<
            Extent != dynamic_extent && N == dynamic_extent && detail::is_qualification_convertible_v<U, T>, int> = 0>
    constexpr explicit span(span<U, N> const& s) noexcept : extent_base(s.size()), m_data(s.data()) {
        detail::precondition(s.size() == Extent, "span: size != extent");
    }

    constexpr span(span const&) noexcept                    = default;
    constexpr auto operator=(span const&) noexcept -> span& = default;

    /// @brief First `Count` elements, static extent.
    template <std::size_t Count>
    constexpr auto first() const {
        static_assert(is_dynamic || Count <= Extent, "span::first: Count > extent");
        detail::precondition(Count <= size(), "span::first: Count > size()");
        return span<T, Count>(m_data, Count);
    }
    /// @brief Last `Count` elements, static extent.
    template <std::size_t Count>
    constexpr auto last() const {
        static_assert(is_dynamic || Count <= Extent, "span::last: Count > extent");
        detail::precondition(Count <= size(), "span::last: Count > size()");
        return span<T, Count>(m_data + (size() - Count), Count);
    }
    /// @brief `Count` elements from `Offset` (or the rest), static extent where computable.
    template <std::size_t Offset, std::size_t Count = dynamic_extent>
    constexpr auto subspan() const {
        static_assert(is_dynamic || Offset <= Extent, "span::subspan: Offset > extent");
        static_assert(is_dynamic || Count == dynamic_extent || Count <= Extent - Offset,
                      "span::subspan: Count > extent - Offset");
        detail::precondition(Offset <= size(), "span::subspan: Offset > size()");
        detail::precondition(Count == dynamic_extent || Count <= size() - Offset,
                             "span::subspan: Count > size() - Offset");
        constexpr std::size_t E = Count != dynamic_extent ? Count : (is_dynamic ? dynamic_extent : Extent - Offset);
        return span<T, E>(m_data + Offset, Count == dynamic_extent ? size() - Offset : Count);
    }

    /// @brief First `count` elements.
    constexpr auto first(size_type count) const {
        detail::precondition(count <= size(), "span::first: count > size()");
        return span<T>(m_data, count);
    }
    /// @brief Last `count` elements.
    constexpr auto last(size_type count) const {
        detail::precondition(count <= size(), "span::last: count > size()");
        return span<T>(m_data + (size() - count), count);
    }
    /// @brief `count` elements from `offset` (or the rest).
    constexpr auto subspan(size_type offset, size_type count = dynamic_extent) const {
        detail::precondition(offset <= size(), "span::subspan: offset > size()");
        detail::precondition(count == dynamic_extent || count <= size() - offset,
                             "span::subspan: count > size() - offset");
        return span<T>(m_data + offset, count == dynamic_extent ? size() - offset : count);
    }

    constexpr auto size() const noexcept { return extent_base::size(); }
    constexpr auto size_bytes() const noexcept { return size() * sizeof(T); }
    [[nodiscard]] constexpr auto empty() const noexcept { return size() == 0; }

    /// @pre `i < size()`.
    constexpr auto operator[](size_type i) const -> reference {
        detail::precondition(i < size(), "span: index out of range");
        return m_data[i];
    }
    /// @pre `!empty()`.
    constexpr auto front() const -> reference {
        detail::precondition(!empty(), "span::front on empty span");
        return m_data[0];
    }
    /// @pre `!empty()`.
    constexpr auto back() const -> reference {
        detail::precondition(!empty(), "span::back on empty span");
        return m_data[size() - 1];
    }
    constexpr auto data() const noexcept { return m_data; }

    constexpr auto begin() const noexcept -> iterator { return m_data; }
    constexpr auto end() const noexcept -> iterator { return m_data + size(); }
    constexpr auto rbegin() const noexcept { return reverse_iterator(end()); }
    constexpr auto rend() const noexcept { return reverse_iterator(begin()); }

private:
    pointer m_data;
};

template <class It, class EndOrSize>
span(It, EndOrSize) -> span<std::remove_reference_t<decltype(*std::declval<It&>())> >;
template <class T, std::size_t N>
span(T (&)[N]) -> span<T, N>;
template <class T, std::size_t N>
span(std::array<T, N>&) -> span<T, N>;
template <class T, std::size_t N>
span(std::array<T, N> const&) -> span<T const, N>;
template <class R>
span(R&&) -> span<std::remove_pointer_t<detail::range_data_t<R> > >;

/// @brief The bytes of `s`, read-only.
template <class T, std::size_t N>
auto as_bytes(span<T, N> s) noexcept {
    constexpr std::size_t E = N == dynamic_extent ? dynamic_extent : N * sizeof(T);
    return span<std::byte const, E>(reinterpret_cast<std::byte const*>(s.data()), s.size_bytes());
}
/// @brief The bytes of `s`, writable; `T` must not be const.
template <class T, std::size_t N, std::enable_if_t<!std::is_const_v<T>, int> = 0>
auto as_writable_bytes(span<T, N> s) noexcept {
    constexpr std::size_t E = N == dynamic_extent ? dynamic_extent : N * sizeof(T);
    return span<std::byte, E>(reinterpret_cast<std::byte*>(s.data()), s.size_bytes());
}
#endif

}  // namespace cp
