#pragma once
/// @file
/// @brief cp::ds::static_vector: a vector with fixed capacity `N` stored inline (`std::inplace_vector` from C++26).
/// @par Caveats
/// - Before C++26 `try_emplace_back`/`try_push_back` return cp::detail::optional_ref (C++26 returns
///   `std::optional<T&>`). Use only `if (r)`, `*r`, `r->`, `has_value()`, `value()` on the result.
/// - No `try_append_range`: C++26 has it, but libstdc++ 16's `std::inplace_vector` does not yet.
/// - `iterator` is a raw pointer in the backport.
/// - The backport is not usable in constant expressions.
/// - Call `erase(v, x)` / `erase_if(v, pred)` unqualified; they are found by argument-dependent lookup.
/// - `operator[]`, `front`, `back`, `pop_back` and `unchecked_*` check their precondition when
///   CP_CHECKS is 1. The C++26 `std::inplace_vector` checks only under the standard library's
///   hardening mode (`_GLIBCXX_ASSERTIONS`).
/// - Clang with libstdc++ 16 at C++26: `std::inplace_vector<T>` of a trivial `T` warns that
///   `is_trivial_v` is deprecated (inside libstdc++); add `-Wno-error=deprecated-declarations`.
#include "cp/config.hpp"
#include <cstddef>
#if defined(__cpp_lib_inplace_vector) && __cpp_lib_inplace_vector >= 202603L  // cp:since c++26
#  include <inplace_vector>
#else
#  include "cp/detail/precondition.hpp"
#  include "cp/detail/range.hpp"
#  include <algorithm>
#  include <array>
#  include <initializer_list>
#  include <iterator>
#  include <memory>
#  include <new>
#  include <optional>
#  include <stdexcept>
#  include <type_traits>
#  include <utility>
#  if defined(__cpp_lib_three_way_comparison)  // cp:since c++20
#    include <compare>
#  endif
#  if defined(__cpp_lib_containers_ranges)  // cp:since c++23: std::from_range_t
#    include <ranges>
#  endif

namespace cp::detail {

/// @brief A reference or nothing; stand-in for C++26 `std::optional<T&>` returned by `try_*_back`.
template <class T>
class optional_ref {
public:
    /// @brief Empty.
    constexpr optional_ref() noexcept = default;
    /// @brief Refers to `x`.
    constexpr explicit optional_ref(T& x) noexcept : m_ptr(std::addressof(x)) { }

    /// @brief True if a reference is held.
    constexpr auto has_value() const noexcept { return m_ptr != nullptr; }
    /// @brief True if a reference is held.
    constexpr explicit operator bool() const noexcept { return m_ptr != nullptr; }
    /// @brief The referenced object.
    constexpr auto operator*() const noexcept -> T& {
        detail::precondition(m_ptr != nullptr, "optional_ref: dereferencing an empty result");
        return *m_ptr;
    }
    /// @brief Pointer to the referenced object.
    constexpr auto operator->() const noexcept {
        detail::precondition(m_ptr != nullptr, "optional_ref: dereferencing an empty result");
        return m_ptr;
    }
    /// @brief The referenced object; throws `std::bad_optional_access` if empty.
    constexpr auto value() const -> T& {
        if (m_ptr == nullptr) throw std::bad_optional_access();
        return *m_ptr;
    }

private:
    T* m_ptr = nullptr;
};

/// @brief Element storage: uninitialised slots for `N` elements and the count of alive ones. For a trivially copyable
/// `T` every special member is the compiler's, so the container is trivially copyable too, as C++26 requires.
template <class T, std::size_t N, bool = std::is_trivially_copyable_v<T>>
struct inplace_storage {
    inplace_storage() noexcept { }
    union {
        std::array<T, N> m_elems;  ///< only [0, m_size) are alive
    };
    std::size_t m_size = 0;  ///< number of alive elements
};

/// @brief Replaces the elements of storage `s` with `n` read from `first`: assigns over the common prefix, then
/// constructs or destroys the rest.
template <class S, class It>
void inplace_assign_n(S& s, It first, std::size_t n) {
    auto* const p     = s.m_elems.data();
    auto const common = std::min(n, s.m_size);
    std::copy_n(first, common, p);
    std::advance(first, common);
    if (n <= s.m_size) {
        std::destroy(p + n, p + s.m_size);
    } else {
        std::uninitialized_copy_n(first, n - common, p + common);
    }
    s.m_size = n;
}

/// @brief Element storage for any other `T`: copies, moves and destroys the alive elements.
template <class T, std::size_t N>
struct inplace_storage<T, N, false> {
    ///@{
    /// @brief Special members over the alive elements.
    inplace_storage() noexcept { }
    inplace_storage(inplace_storage const& o) noexcept(std::is_nothrow_copy_constructible_v<T>) {
        std::uninitialized_copy_n(o.m_elems.data(), o.m_size, m_elems.data());
        m_size = o.m_size;
    }
    inplace_storage(inplace_storage&& o) noexcept(std::is_nothrow_move_constructible_v<T>) {
        std::uninitialized_move_n(o.m_elems.data(), o.m_size, m_elems.data());
        m_size = o.m_size;
    }
    auto operator=(inplace_storage const& o) -> inplace_storage& {
        if (this != &o) inplace_assign_n(*this, o.m_elems.data(), o.m_size);
        return *this;
    }
    auto operator=(inplace_storage&& o) noexcept(std::is_nothrow_move_constructible_v<T> &&
                                                 std::is_nothrow_move_assignable_v<T>) -> inplace_storage& {
        if (this != &o) inplace_assign_n(*this, std::make_move_iterator(o.m_elems.data()), o.m_size);
        return *this;
    }
    ~inplace_storage() { std::destroy_n(m_elems.data(), m_size); }
    ///@}
    union {
        std::array<T, N> m_elems;  ///< only [0, m_size) are alive
    };
    std::size_t m_size = 0;  ///< number of alive elements
};

/// @brief Backport of C++26 `std::inplace_vector<T, N>`; see cp::ds::static_vector.
template <class T, std::size_t N>
class static_vector_backport {
public:
    using value_type             = T;                                      ///< element type
    using pointer                = T*;                                     ///< `T*`
    using const_pointer          = T const*;                               ///< `T const*`
    using reference              = T&;                                     ///< `T&`
    using const_reference        = T const&;                               ///< `T const&`
    using size_type              = std::size_t;                            ///< `std::size_t`
    using difference_type        = std::ptrdiff_t;                         ///< `std::ptrdiff_t`
    using iterator               = T*;                                     ///< raw pointer
    using const_iterator         = T const*;                               ///< raw pointer
    using reverse_iterator       = std::reverse_iterator<iterator>;        ///< reverse iterator
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;  ///< const reverse iterator

    // ---- construct / copy ---------------------------------------------------

    /// @brief Empty.
    static_vector_backport() noexcept = default;
    /// @brief `n` value-initialised elements; throws `std::bad_alloc` if `n > N`.
    explicit static_vector_backport(size_type n) {
        reserve(n);
        std::uninitialized_value_construct_n(m_ptr(), n);
        m_set_size(n);
    }
    /// @brief `n` copies of `value`; throws `std::bad_alloc` if `n > N`.
    static_vector_backport(size_type n, T const& value) {
        reserve(n);
        std::uninitialized_fill_n(m_ptr(), n, value);
        m_set_size(n);
    }
    /// @brief Copies `[first, last)`; throws `std::bad_alloc` if it does not fit.
    template <class It, std::enable_if_t<is_input_iterator_v<It>, int> = 0>
    static_vector_backport(It first, It last) {
        m_append(first, last);
    }
#  if defined(__cpp_lib_containers_ranges)  // cp:since c++23
    /// @brief Copies the range `r`; throws `std::bad_alloc` if it does not fit.
    template <class R, std::enable_if_t<is_range_v<R>, int> = 0>
    static_vector_backport(std::from_range_t, R&& r) {
        append_range(std::forward<R>(r));
    }
#  endif
    /// @brief Copies `il`; throws `std::bad_alloc` if it does not fit.
    static_vector_backport(std::initializer_list<T> il) : static_vector_backport(il.begin(), il.end()) { }

    /// @brief Replaces the contents with `il`.
    auto operator=(std::initializer_list<T> il) -> static_vector_backport& {
        assign(il.begin(), il.end());
        return *this;
    }
    /// @brief Replaces the contents with `[first, last)`; throws `std::bad_alloc` if it does not fit.
    template <class It, std::enable_if_t<is_input_iterator_v<It>, int> = 0>
    void assign(It first, It last) {
        m_assign_from(first, last);
    }
    /// @brief Replaces the contents with the range `r`.
    template <class R, std::enable_if_t<is_range_v<R>, int> = 0>
    void assign_range(R&& r) {
        m_assign_from(range_begin(r), range_end(r));
    }
    /// @brief Replaces the contents with `n` copies of `value`.
    void assign(size_type n, T const& value) {
        reserve(n);
        clear();
        std::uninitialized_fill_n(m_ptr(), n, value);
        m_set_size(n);
    }
    /// @brief Replaces the contents with `il`.
    void assign(std::initializer_list<T> il) { assign(il.begin(), il.end()); }

    // ---- iterators ----------------------------------------------------------

    /// @brief Iterator to the first element.
    auto begin() noexcept { return m_ptr(); }
    /// @brief Iterator to the first element.
    auto begin() const noexcept { return m_ptr(); }
    /// @brief Iterator past the last element.
    auto end() noexcept { return m_ptr() + size(); }
    /// @brief Iterator past the last element.
    auto end() const noexcept { return m_ptr() + size(); }
    /// @brief Reverse iterator to the last element.
    auto rbegin() noexcept { return reverse_iterator(end()); }
    /// @brief Reverse iterator to the last element.
    auto rbegin() const noexcept { return const_reverse_iterator(end()); }
    /// @brief Reverse iterator before the first element.
    auto rend() noexcept { return reverse_iterator(begin()); }
    /// @brief Reverse iterator before the first element.
    auto rend() const noexcept { return const_reverse_iterator(begin()); }
    /// @brief Const iterator to the first element.
    auto cbegin() const noexcept { return begin(); }
    /// @brief Const iterator past the last element.
    auto cend() const noexcept { return end(); }
    /// @brief Const reverse iterator to the last element.
    auto crbegin() const noexcept { return rbegin(); }
    /// @brief Const reverse iterator before the first element.
    auto crend() const noexcept { return rend(); }

    // ---- size / capacity ----------------------------------------------------

    /// @brief True if there are no elements.
    auto empty() const noexcept { return size() == 0; }
    /// @brief Number of elements.
    auto size() const noexcept { return m_get_size(); }
    /// @brief `N`.
    static constexpr auto max_size() noexcept { return N; }
    /// @brief `N`.
    static constexpr auto capacity() noexcept { return N; }
    /// @brief Grows with value-initialised elements or shrinks to `n`; throws `std::bad_alloc` if `n > N`.
    void resize(size_type n) {
        reserve(n);
        if (n <= size()) return m_shrink_to(n);
        std::uninitialized_value_construct(end(), begin() + n);
        m_set_size(n);
    }
    /// @brief Grows with copies of `value` or shrinks to `n`; throws `std::bad_alloc` if `n > N`.
    void resize(size_type n, T const& value) {
        reserve(n);
        if (n <= size()) return m_shrink_to(n);
        std::uninitialized_fill(end(), begin() + n, value);
        m_set_size(n);
    }
    /// @brief Throws `std::bad_alloc` if `n > N`; otherwise does nothing.
    static void reserve(size_type n) {
        if (n > N) throw std::bad_alloc();
    }
    /// @brief Does nothing.
    static void shrink_to_fit() noexcept { }

    // ---- element access -----------------------------------------------------

    /// @brief Element `i`.
    /// @pre `i < size()`.
    auto operator[](size_type i) -> T& {
        detail::precondition(i < size(), "static_vector: index out of range");
        return begin()[i];
    }
    /// @brief Element `i`.
    /// @pre `i < size()`.
    auto operator[](size_type i) const -> T const& {
        detail::precondition(i < size(), "static_vector: index out of range");
        return begin()[i];
    }
    /// @brief Element `i`; throws `std::out_of_range` if `i >= size()`.
    auto at(size_type i) -> T& {
        if (i >= size()) throw std::out_of_range("static_vector::at");
        return begin()[i];
    }
    /// @brief Element `i`; throws `std::out_of_range` if `i >= size()`.
    auto at(size_type i) const -> T const& {
        if (i >= size()) throw std::out_of_range("static_vector::at");
        return begin()[i];
    }
    /// @brief First element.
    /// @pre `!empty()`.
    auto front() -> T& {
        detail::precondition(!empty(), "static_vector::front on empty vector");
        return *begin();
    }
    /// @brief First element.
    /// @pre `!empty()`.
    auto front() const -> T const& {
        detail::precondition(!empty(), "static_vector::front on empty vector");
        return *begin();
    }
    /// @brief Last element.
    /// @pre `!empty()`.
    auto back() -> T& {
        detail::precondition(!empty(), "static_vector::back on empty vector");
        return end()[-1];
    }
    /// @brief Last element.
    /// @pre `!empty()`.
    auto back() const -> T const& {
        detail::precondition(!empty(), "static_vector::back on empty vector");
        return end()[-1];
    }
    /// @brief Pointer to the first element (`nullptr` when N == 0).
    auto data() noexcept { return m_ptr(); }
    /// @brief Pointer to the first element (`nullptr` when N == 0).
    auto data() const noexcept { return m_ptr(); }

    // ---- appending ----------------------------------------------------------

    /// @brief Constructs a last element from `args`; throws `std::bad_alloc` if full.
    template <class... Args>
    auto emplace_back(Args&&... args) -> T& {
        if (size() == N) throw std::bad_alloc();
        return m_construct_back(std::forward<Args>(args)...);
    }
    /// @brief Appends a copy of `x`; throws `std::bad_alloc` if full.
    auto push_back(T const& x) -> T& { return emplace_back(x); }
    /// @brief Appends `x` by move; throws `std::bad_alloc` if full.
    auto push_back(T&& x) -> T& { return emplace_back(std::move(x)); }
    /// @brief Appends the range `r`; throws `std::bad_alloc` if it does not fit (a forward
    /// range is checked first; elements of a single-pass range appended before that stay).
    template <class R, std::enable_if_t<is_range_v<R>, int> = 0>
    void append_range(R&& r) {
        m_append(range_begin(r), range_end(r));
    }
    /// @brief Removes the last element.
    /// @pre `!empty()`.
    void pop_back() {
        detail::precondition(!empty(), "static_vector::pop_back on empty vector");
        m_shrink_to(size() - 1);
    }

    /// @brief Constructs a last element from `args` if not full; returns it, or nothing.
    template <class... Args>
    auto try_emplace_back(Args&&... args) -> optional_ref<T> {
        if (size() == N) return {};
        return optional_ref<T>(m_construct_back(std::forward<Args>(args)...));
    }
    /// @brief Appends a copy of `x` if not full; returns it, or nothing.
    auto try_push_back(T const& x) { return try_emplace_back(x); }
    /// @brief Appends `x` by move if not full; returns it, or nothing.
    auto try_push_back(T&& x) { return try_emplace_back(std::move(x)); }

    /// @brief Constructs a last element from `args`.
    /// @pre `size() < N`.
    template <class... Args>
    auto unchecked_emplace_back(Args&&... args) -> T& {
        detail::precondition(size() < N, "static_vector::unchecked_emplace_back on full vector");
        return m_construct_back(std::forward<Args>(args)...);
    }
    /// @brief Appends a copy of `x`.
    /// @pre `size() < N`.
    auto unchecked_push_back(T const& x) -> T& { return unchecked_emplace_back(x); }
    /// @brief Appends `x` by move.
    /// @pre `size() < N`.
    auto unchecked_push_back(T&& x) -> T& { return unchecked_emplace_back(std::move(x)); }

    // ---- inserting / erasing ------------------------------------------------
    // Inserting appends at the end, then rotates into place. Nothing moves before the new
    // elements are built, so arguments that refer into the vector stay valid.

    /// @brief Constructs an element from `args` before `pos`; throws `std::bad_alloc` if full.
    template <class... Args>
    auto emplace(const_iterator pos, Args&&... args) -> iterator {
        auto i = m_check_position(pos);
        emplace_back(std::forward<Args>(args)...);
        return m_rotate_in(i);
    }
    /// @brief Inserts a copy of `x` before `pos`.
    auto insert(const_iterator pos, T const& x) -> iterator { return emplace(pos, x); }
    /// @brief Inserts `x` by move before `pos`.
    auto insert(const_iterator pos, T&& x) -> iterator { return emplace(pos, std::move(x)); }
    /// @brief Inserts `n` copies of `x` before `pos`; throws `std::bad_alloc` if they do not fit.
    auto insert(const_iterator pos, size_type n, T const& x) -> iterator {
        auto i   = m_check_position(pos);
        auto old = size();
        reserve(old + n);
        std::uninitialized_fill_n(end(), n, x);
        m_set_size(old + n);
        return m_rotate_in(i, old);
    }
    /// @brief Inserts `[first, last)` before `pos`; throws `std::bad_alloc` if it does not fit.
    template <class It, std::enable_if_t<is_input_iterator_v<It>, int> = 0>
    auto insert(const_iterator pos, It first, It last) -> iterator {
        return m_insert_from(pos, first, last);
    }
    /// @brief Inserts the range `r` before `pos`; throws `std::bad_alloc` if it does not fit.
    template <class R, std::enable_if_t<is_range_v<R>, int> = 0>
    auto insert_range(const_iterator pos, R&& r) -> iterator {
        return m_insert_from(pos, range_begin(r), range_end(r));
    }
    /// @brief Inserts `il` before `pos`.
    auto insert(const_iterator pos, std::initializer_list<T> il) -> iterator {
        return insert(pos, il.begin(), il.end());
    }
    /// @brief Removes the element at `pos`; returns the iterator after it.
    /// @pre `pos` points to an element.
    auto erase(const_iterator pos) -> iterator {
        return erase(pos, pos + 1);  // which checks the position
    }
    /// @brief Removes `[first, last)`; returns the iterator after the removed elements.
    /// @pre `[first, last)` is a range inside the vector.
    auto erase(const_iterator first, const_iterator last) -> iterator {
        detail::precondition(begin() <= first && first <= last && last <= end(), "static_vector::erase: invalid range");
        auto f = begin() + (first - begin());
        if (first != last) {
            auto new_end = std::move(f + (last - first), end(), f);
            m_shrink_to(static_cast<size_type>(new_end - begin()));
        }
        return f;
    }
    /// @brief Exchanges contents with `other`.
    void swap(static_vector_backport& other) noexcept(std::is_nothrow_swappable_v<T> &&
                                                      std::is_nothrow_move_constructible_v<T>) {
        auto& small = size() <= other.size() ? *this : other;
        auto& big   = size() <= other.size() ? other : *this;
        auto n      = small.size();
        auto n2     = big.size();
        if constexpr (std::is_nothrow_move_constructible_v<T>) {
            std::uninitialized_move(big.begin() + n, big.end(), small.end());
        } else {
            std::uninitialized_copy(big.begin() + n, big.end(), small.end());
        }
        small.m_set_size(n2);
        big.m_shrink_to(n);
        std::swap_ranges(begin(), begin() + n, other.begin());
    }
    /// @brief Removes every element.
    void clear() noexcept { m_shrink_to(0); }

    // ---- comparison ---------------------------------------------------------

    /// @brief Element-wise equality.
    friend auto operator==(static_vector_backport const& x, static_vector_backport const& y) -> bool {
        return std::equal(x.begin(), x.end(), y.begin(), y.end());
    }
#  if defined(__cpp_lib_three_way_comparison)  // cp:since c++20
    /// @brief Lexicographic three-way comparison (falls back to `<` when `T` has no `<=>`).
    friend auto operator<=>(static_vector_backport const& x, static_vector_backport const& y) {
        return std::lexicographical_compare_three_way(x.begin(), x.end(), y.begin(), y.end(), synth_three_way);
    }
#  else
    /// @brief Element-wise inequality.
    friend auto operator!=(static_vector_backport const& x, static_vector_backport const& y) -> bool {
        return !(x == y);
    }
    /// @brief Lexicographic `<`.
    friend auto operator<(static_vector_backport const& x, static_vector_backport const& y) -> bool {
        return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
    }
    /// @brief Lexicographic `>`.
    friend auto operator>(static_vector_backport const& x, static_vector_backport const& y) -> bool { return y < x; }
    /// @brief Lexicographic `<=`.
    friend auto operator<=(static_vector_backport const& x, static_vector_backport const& y) -> bool {
        return !(y < x);
    }
    /// @brief Lexicographic `>=`.
    friend auto operator>=(static_vector_backport const& x, static_vector_backport const& y) -> bool {
        return !(x < y);
    }
#  endif
    /// @brief Exchanges contents.
    friend void swap(static_vector_backport& x, static_vector_backport& y) noexcept(noexcept(x.swap(y))) { x.swap(y); }

private:
    auto m_ptr() noexcept -> T* { return m_s.m_elems.data(); }
    auto m_ptr() const noexcept -> T const* { return m_s.m_elems.data(); }
    auto m_get_size() const noexcept -> std::size_t { return m_s.m_size; }
    void m_set_size(std::size_t n) noexcept { m_s.m_size = n; }
    /// @brief Constructs a new last element without checking capacity.
    template <class... Args>
    auto m_construct_back(Args&&... args) -> T& {
        auto* p = ::new (static_cast<void*>(m_ptr() + m_get_size())) T(std::forward<Args>(args)...);
        m_set_size(m_get_size() + 1);
        return *p;
    }
    /// @brief Destroys the elements from index `n` on.
    void m_shrink_to(std::size_t n) noexcept {
        std::destroy(m_ptr() + n, m_ptr() + m_get_size());
        m_set_size(n);
    }
    /// @brief Replaces the contents with `n` elements read from `first`.
    template <class It>
    void m_assign_n(It first, std::size_t n) {
        inplace_assign_n(m_s, first, n);
    }

#  if defined(__cpp_lib_three_way_comparison)  // cp:since c++20
    /// @brief `<=>` if `T` has it, else an ordering derived from `<` (the standard's synth-three-way).
    static constexpr auto synth_three_way = [](T const& a, T const& b) {
        if constexpr (std::three_way_comparable<T>) {
            return a <=> b;
        } else {
            if (a < b) return std::weak_ordering::less;
            if (b < a) return std::weak_ordering::greater;
            return std::weak_ordering::equivalent;
        }
    };
#  endif

    /// @brief Appends `[first, last)`; a multi-pass range is checked against the capacity first.
    template <class It, class Sentinel>
    void m_append(It first, Sentinel last) {
        if constexpr (is_forward_iterator_v<It>) {
            reserve(size() + iter_distance(first, last));
            for (; first != last; ++first) m_construct_back(*first);
        } else {
            for (; first != last; ++first) emplace_back(*first);
        }
    }
    /// @brief Replaces the contents with `[first, last)`.
    template <class It, class Sentinel>
    void m_assign_from(It first, Sentinel last) {
        if constexpr (is_forward_iterator_v<It>) {
            auto n = iter_distance(first, last);
            reserve(n);
            m_assign_n(first, n);
        } else {
            clear();
            m_append(first, last);
        }
    }
    /// @brief Inserts `[first, last)` before `pos`; on failure the vector is unchanged.
    template <class It, class Sentinel>
    auto m_insert_from(const_iterator pos, It first, Sentinel last) -> iterator {
        auto i   = m_check_position(pos);
        auto old = size();
        try {
            m_append(first, last);
        } catch (...) {
            m_shrink_to(old);
            throw;
        }
        return m_rotate_in(i, old);
    }
    /// @brief Checks `pos` and converts it to an index.
    auto m_check_position(const_iterator pos) const -> size_type {
        detail::precondition(begin() <= pos && pos <= end(), "static_vector::insert: position out of range");
        return static_cast<size_type>(pos - begin());
    }
    /// @brief Rotates the elements appended after index `old` to index `i`; returns an iterator to them.
    auto m_rotate_in(size_type i, size_type old) -> iterator {
        std::rotate(begin() + i, begin() + old, end());
        return begin() + i;
    }
    /// @brief Rotates the one element just appended to index `i`.
    auto m_rotate_in(size_type i) -> iterator { return m_rotate_in(i, size() - 1); }

    inplace_storage<T, N> m_s;
};

/// @brief Removes every element equal to `value`; returns how many were removed.
template <class T, std::size_t N, class U = T>
auto erase(static_vector_backport<T, N>& c, U const& value) {
    auto it = std::remove(c.begin(), c.end(), value);
    auto n  = static_cast<std::size_t>(c.end() - it);
    c.erase(it, c.end());
    return n;
}
/// @brief Removes every element satisfying `pred`; returns how many were removed.
template <class T, std::size_t N, class Pred>
auto erase_if(static_vector_backport<T, N>& c, Pred pred) {
    auto it = std::remove_if(c.begin(), c.end(), pred);
    auto n  = static_cast<std::size_t>(c.end() - it);
    c.erase(it, c.end());
    return n;
}

}  // namespace cp::detail
#endif

namespace cp::ds {

/// @brief Vector of at most `N` elements stored inline (no heap); full operations throw `std::bad_alloc`.
/// `std::inplace_vector` from C++26.
template <class T, std::size_t N>
#if defined(__cpp_lib_inplace_vector) && __cpp_lib_inplace_vector >= 202603L  // cp:since c++26
using static_vector = std::inplace_vector<T, N>;
#else
using static_vector = detail::static_vector_backport<T, N>;
#endif

}  // namespace cp::ds
