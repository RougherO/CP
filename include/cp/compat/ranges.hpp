#pragma once
/// @file
/// @brief `cp::views`: `std::views` in C++20; `iota` and `reverse` backports in C++17.
/// @par Caveats
/// - C++17 `cp::views` has only `iota` and `reverse`.
/// - Binary search (`std::partition_point`, `std::lower_bound`) over `iota` is O(log n): the
///   C++17 backport is random access, and libstdc++ 16 dispatches C++20 `std::views::iota` on
///   `iterator_concept`. Older standard libraries may step linearly (iota's `iterator_category`
///   is `input` by the standard); `std::ranges::partition_point` is O(log n) everywhere.
/// - As in std, `iota(a, b)` needs bounds of the same signedness (`iota(0, v.size())` does not
///   compile), and different types (`iota(0, 5LL)`) give an end sentinel that classic
///   algorithms reject. Use one type for both bounds.
#include "cp/compat/version.hpp"
#if defined(__cpp_lib_ranges)  // cp:since c++20
#  include <ranges>
#else
#  include "cp/compat/meta.hpp"
#  include "cp/compat/utility.hpp"
#  include "cp/detail/precondition.hpp"
#  include <cstddef>
#  include <iterator>
#  include <type_traits>
#  include <utility>
#endif

namespace cp {

#if defined(__cpp_lib_ranges)  // cp:since c++20
namespace views = std::views;
#else
namespace detail {

/// @brief Sentinel of an unbounded iota: never equal to any iterator.
struct unreachable_sentinel { };

/// @brief `[first, last)` of consecutive integers. `Bound` is `W` (common range), another
/// integer type (sentinel end, as in std) or unreachable_sentinel.
template <class W, class Bound>
class iota_view {
    static_assert(is_integer_v<W>, "cp::views::iota requires an integer type");
    static constexpr bool is_common  = std::is_same_v<W, Bound>;
    static constexpr bool is_bounded = !std::is_same_v<Bound, unreachable_sentinel>;

public:
    /// @brief Random-access iterator yielding values (not references).
    class iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type        = W;
        using difference_type   = std::conditional_t<(sizeof(W) < sizeof(long long)), long long, std::ptrdiff_t>;
        using pointer           = void;
        using reference         = W;

        constexpr iterator() noexcept = default;
        constexpr explicit iterator(W value) noexcept : m_value(value) { }

        constexpr auto operator*() const noexcept { return m_value; }
        constexpr auto operator[](difference_type n) const noexcept { return W(m_value + n); }

        constexpr auto operator++() noexcept -> iterator& {
            ++m_value;
            return *this;
        }
        constexpr auto operator--() noexcept -> iterator& {
            --m_value;
            return *this;
        }
        constexpr auto operator++(int) noexcept {
            auto t = *this;
            ++m_value;
            return t;
        }
        constexpr auto operator--(int) noexcept {
            auto t = *this;
            --m_value;
            return t;
        }
        constexpr auto operator+=(difference_type n) noexcept -> iterator& {
            m_value = W(m_value + n);
            return *this;
        }
        constexpr auto operator-=(difference_type n) noexcept -> iterator& {
            m_value = W(m_value - n);
            return *this;
        }

        friend constexpr auto operator+(iterator it, difference_type n) noexcept { return it += n; }
        friend constexpr auto operator+(difference_type n, iterator it) noexcept { return it += n; }
        friend constexpr auto operator-(iterator it, difference_type n) noexcept { return it -= n; }
        friend constexpr auto operator-(iterator a, iterator b) noexcept {
            // unsigned W: subtract in the wider signed type so b > a gives a negative distance
            return difference_type(a.m_value) - difference_type(b.m_value);
        }
        friend constexpr auto operator==(iterator a, iterator b) noexcept { return a.m_value == b.m_value; }
        friend constexpr auto operator!=(iterator a, iterator b) noexcept { return a.m_value != b.m_value; }
        friend constexpr auto operator<(iterator a, iterator b) noexcept { return a.m_value < b.m_value; }
        friend constexpr auto operator>(iterator a, iterator b) noexcept { return a.m_value > b.m_value; }
        friend constexpr auto operator<=(iterator a, iterator b) noexcept { return a.m_value <= b.m_value; }
        friend constexpr auto operator>=(iterator a, iterator b) noexcept { return a.m_value >= b.m_value; }
        friend constexpr auto operator==(iterator, unreachable_sentinel) noexcept { return false; }
        friend constexpr auto operator!=(iterator, unreachable_sentinel) noexcept { return true; }

    private:
        W m_value{};
    };

    /// @brief End of a view whose bound type differs from `W`; compares with the usual conversions.
    class sentinel {
    public:
        constexpr sentinel() noexcept = default;
        constexpr explicit sentinel(Bound bound) noexcept : m_bound(bound) { }

        friend constexpr auto operator==(iterator it, sentinel s) noexcept { return s.equals(it); }
        friend constexpr auto operator==(sentinel s, iterator it) noexcept { return s.equals(it); }
        friend constexpr auto operator!=(iterator it, sentinel s) noexcept { return !s.equals(it); }
        friend constexpr auto operator!=(sentinel s, iterator it) noexcept { return !s.equals(it); }

    private:
        constexpr auto equals(iterator it) const noexcept {
            using C = std::common_type_t<W, Bound>;  // what `value == bound` does, minus -Wsign-compare
            return static_cast<C>(*it) == static_cast<C>(m_bound);
        }
        Bound m_bound{};
    };

    constexpr iota_view(W first, Bound last) noexcept : m_first(first), m_last(last) { }

    constexpr auto begin() const noexcept { return iterator(m_first); }
    constexpr auto end() const noexcept {
        if constexpr (is_common) {
            return iterator(m_last);
        } else if constexpr (is_bounded) {
            return sentinel(m_last);
        } else {
            return unreachable_sentinel{};
        }
    }
    /// @brief Number of values; bounded views only.
    template <bool B = is_bounded, std::enable_if_t<B, int> = 0>
    constexpr auto size() const noexcept {
        using U = std::make_unsigned_t<std::common_type_t<W, Bound> >;
        return static_cast<U>(static_cast<U>(m_last) - static_cast<U>(m_first));
    }
    template <bool B = is_bounded, std::enable_if_t<B, int> = 0>
    [[nodiscard]] constexpr auto empty() const noexcept {
        return begin() == end();
    }

private:
    W m_first;
    Bound m_last;
};

/// @brief Function object behind cp::views::iota.
struct iota_fn {
    /// @brief `first, first + 1, ...` without end.
    template <class W>
    constexpr auto operator()(W first) const noexcept {
        return iota_view<W, unreachable_sentinel>(first, {});
    }
    /// @brief `first, ..., last - 1`; bounds must have the same signedness.
    /// @pre `first <= last`.
    template <
        class W, class B,
        std::enable_if_t<is_integer_v<W> && is_integer_v<B> && std::is_signed_v<W> == std::is_signed_v<B>, int> = 0>
    constexpr auto operator()(W first, B last) const {
        precondition(cmp_less_equal(first, last), "views::iota: first > last");
        return iota_view<W, B>(first, last);
    }
};

/// @brief Reversed view of a bidirectional range; owns the range if given an rvalue.
template <class R>
class reverse_view {
public:
    constexpr explicit reverse_view(R&& r) : m_range(std::forward<R>(r)) { }

    constexpr auto begin() { return std::make_reverse_iterator(std::end(m_range)); }
    constexpr auto end() { return std::make_reverse_iterator(std::begin(m_range)); }
    constexpr auto begin() const { return std::make_reverse_iterator(std::end(m_range)); }
    constexpr auto end() const { return std::make_reverse_iterator(std::begin(m_range)); }
    constexpr auto size() const { return std::size(m_range); }
    [[nodiscard]] constexpr auto empty() const { return std::begin(m_range) == std::end(m_range); }

private:
    R m_range;  // reference for lvalues, value for rvalues
};

/// @brief Function object behind cp::views::reverse; also usable as `r | views::reverse`.
struct reverse_fn {
    template <class R>
    constexpr auto operator()(R&& r) const {
        return reverse_view<R>(std::forward<R>(r));
    }
    template <class R>
    friend constexpr auto operator|(R&& r, reverse_fn f) {
        return f(std::forward<R>(r));
    }
};

}  // namespace detail

/// @brief Range adaptors: `std::views` in C++20, the C++17 subset here otherwise.
namespace views {
/// @brief `iota(a)`: a, a+1, ... ; `iota(a, b)`: a, ..., b-1.
inline constexpr detail::iota_fn iota{};
/// @brief `reverse(r)` or `r | reverse`: `r` back to front.
inline constexpr detail::reverse_fn reverse{};
}  // namespace views
#endif

}  // namespace cp
