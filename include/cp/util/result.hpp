#pragma once
/// @file
/// @brief cp::util::result<T, E>: a value of `T` (nothing for `void`) or an error `E`, like Rust's `Result<T, E>`, with
/// cp::util::success and cp::util::failure as its `Ok` and `Err`: `return success(5);`, `return failure(e);` (a plain
/// value converts in too: `return 5;`).
/// @par Caveats
/// - C++23 and later: result derives from `std::expected<T, E>` and failure from `std::unexpected<E>`, so every
///   `std::expected` member is there, monadic ones included (`and_then`, `transform`, `or_else`,
///   `transform_error`; they return `std::expected`). Its rules come along too: emplace() takes only constructions that
///   cannot throw. Deriving, not aliasing, drops libstdc++'s `[[nodiscard]]` on
///   `std::expected`, which would make every `io::read(x);` statement a warning.
/// - C++17/20: cp's own class with C++17 `std::optional`'s interface, an error in place of "empty": construction,
///   `in_place`, assignment, emplace(), swap(), has_value(), `explicit operator bool`, `*`, `->`, value(), value_or(),
///   `==`, `std::hash`; plus error(). No monadic members, as `std::optional` has none before C++23.
/// - Not `std`'s names: `unexpected` clashes with libstdc++'s old `std::unexpected()` before C++23 and `unexpect` with
///   C++23's tag, once `using namespace std;` meets `using namespace cp::util;`.
/// - `*`, `->` or value() without a value, or error() without an error, is a bug. Before C++23 with `CP_CHECKS` the
///   program ends with a report on standard error, otherwise it is undefined; C++23's value() throws
///   `std::bad_expected_access`, as `std::expected` does.
#include "cp/config.hpp"
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
#if defined(__cpp_lib_expected)  // cp:since c++23
#  include <expected>
#else
#  include "cp/detail/precondition.hpp"
#  include "cp/traits/traits.hpp"
#  include <optional>
#  include <variant>
#endif

/// @namespace cp::util
/// @brief General utilities: result, success, failure.
namespace cp::util {

/// @brief A value on its way into a result: `return success(x);` (Rust's `Ok`).
template <class T = void>
class success {
public:
    /// @brief Holds `value`.
    constexpr explicit success(T value) : m_value(std::move(value)) { }
    ///@{
    /// @brief The value.
    constexpr auto value() & noexcept -> T& { return m_value; }
    constexpr auto value() const& noexcept -> T const& { return m_value; }
    constexpr auto value() && noexcept -> T&& { return std::move(m_value); }
    ///@}

private:
    T m_value;
};
/// @brief Success without a value, for `result<void, E>`: `return success();`.
template <>
class success<void> { };
///@{
/// @brief Deduction guides.
template <class T>
success(T) -> success<T>;
success() -> success<void>;
///@}

#if defined(__cpp_lib_expected)  // cp:since c++23
/// @brief An error on its way into a result: `return failure(e);` (Rust's `Err`). A `std::unexpected<E>`.
template <class E>
class failure : public std::unexpected<E> {
public:
    using std::unexpected<E>::unexpected;
};
/// @brief Deduction guide.
template <class E>
failure(E) -> failure<E>;

/// @brief A value of `T` (nothing for `void`) or an error `E`: a `std::expected<T, E>` without its `[[nodiscard]]`.
template <class T, class E>
class result : public std::expected<T, E> {
    using base = std::expected<T, E>;

public:
    using base::base;
    using base::operator=;
    /// @brief Holds the value of `s`.
    template <class U>
        requires(!std::is_void_v<T> && std::is_constructible_v<T, U&&>)
    constexpr result(success<U> s) : base(std::in_place, std::move(s).value()) { }  // NOLINT(*-explicit-*)
    /// @brief Success.
    constexpr result(success<void> /*s*/)  // NOLINT(*-explicit-*)
        requires std::is_void_v<T>
        : base() { }
    /// @brief Holds the error of `f`.
    template <class G>
    constexpr result(failure<G> f) : base(std::unexpect, std::move(f).error()) { }  // NOLINT(*-explicit-*)
    /// @brief The same contents as `e`.
    constexpr result(base e) : base(std::move(e)) { }  // NOLINT(*-explicit-*)

    /// @brief Equal if both hold equal values or both hold equal errors. An exact match: through the base class,
    /// `std::expected`'s own `==` would be ambiguous with its reversed form.
    template <class L, class R>  // both deduced: only result == result, no conversion that competes with base's `==`
        requires std::is_same_v<L, result> && std::is_same_v<R, result>
    friend constexpr auto operator==(L const& a, R const& b) -> bool {
        return static_cast<base const&>(a) == static_cast<base const&>(b);
    }
};
#else
/// @brief An error on its way into a result: `return failure(e);` (Rust's `Err`).
template <class E>
class failure {
public:
    /// @brief Holds `e`.
    constexpr explicit failure(E e) : m_error(std::move(e)) { }
    ///@{
    /// @brief The error.
    constexpr auto error() & noexcept -> E& { return m_error; }
    constexpr auto error() const& noexcept -> E const& { return m_error; }
    constexpr auto error() && noexcept -> E&& { return std::move(m_error); }
    ///@}

private:
    E m_error;
};
/// @brief Deduction guide.
template <class E>
failure(E) -> failure<E>;

/// @brief A value of `T` or an error `E`. `std::optional`'s C++17 interface, with an error in place of "empty".
template <class T, class E>
class result {
    /// True if the catch-all constructor, assignment and `== value` take `U` as the value: anything but a result,
    /// success, failure or `std::in_place`, which have their own overloads (without this, `result r2 = r1;` on a
    /// non-const `r1` would pick the catch-all over the copy constructor). `std::optional`/`std::expected` guard alike.
    template <class U, class V = traits::remove_cvref_t<U>>
    static constexpr bool takes_as_value =
        !traits::is_specialization_of_v<V, cp::util::result> && !traits::is_specialization_of_v<V, success> &&
        !traits::is_specialization_of_v<V, failure> && !std::is_same_v<V, std::in_place_t>;
    /// True if a result whose value and error arrive as `U` and `G` (cvref-qualified) converts in; implicit says
    /// whether it does so implicitly.
    template <class U, class G>
    static constexpr bool converts = std::is_constructible_v<T, U> && std::is_constructible_v<E, G>;
    template <class U, class G>
    static constexpr bool implicit = std::is_convertible_v<U, T> && std::is_convertible_v<G, E>;

public:
    /// @brief Holds a value-initialised `T`.
    constexpr result() = default;
    /// @brief Holds `value`.
    template <class U = T, std::enable_if_t<takes_as_value<U> && std::is_constructible_v<T, U&&>, int> = 0>
    constexpr result(U&& value) : m_v(std::in_place_index<0>, std::forward<U>(value)) { }  // NOLINT(*-explicit-*)
    /// @brief Holds the value of `s`.
    template <class U, std::enable_if_t<std::is_constructible_v<T, U&&>, int> = 0>
    constexpr result(success<U> s) : m_v(std::in_place_index<0>, std::move(s).value()) { }  // NOLINT(*-explicit-*)
    /// @brief Holds a `T` built from `args`.
    template <class... Args>
    constexpr explicit result(std::in_place_t /*tag*/, Args&&... args)
        : m_v(std::in_place_index<0>, std::forward<Args>(args)...) { }
    /// @brief Holds the error of `f`.
    template <class G>
    constexpr result(failure<G> f) : m_v(std::in_place_index<1>, std::move(f).error()) { }  // NOLINT(*-explicit-*)
    ///@{
    /// @brief The value or error of `other`, converted; explicit unless both convert implicitly, as `std::expected`.
    template <class U, class G, std::enable_if_t<converts<U const&, G const&> && implicit<U const&, G const&>, int> = 0>
    constexpr result(result<U, G> const& other) : m_v(convert(other)) { }  // NOLINT(*-explicit-*)
    template <class U, class G,
              std::enable_if_t<converts<U const&, G const&> && !implicit<U const&, G const&>, int> = 0>
    constexpr explicit result(result<U, G> const& other) : m_v(convert(other)) { }
    template <class U, class G, std::enable_if_t<converts<U, G> && implicit<U, G>, int> = 0>
    constexpr result(result<U, G>&& other) : m_v(convert(std::move(other))) { }  // NOLINT(*-explicit-*)
    template <class U, class G, std::enable_if_t<converts<U, G> && !implicit<U, G>, int> = 0>
    constexpr explicit result(result<U, G>&& other) : m_v(convert(std::move(other))) { }
    ///@}

    /// @brief Now holds `value`.
    template <class U = T, std::enable_if_t<takes_as_value<U> && std::is_constructible_v<T, U&&>, int> = 0>
    auto operator=(U&& value) -> result& {
        m_v.template emplace<0>(std::forward<U>(value));
        return *this;
    }
    /// @brief Now holds the value of `s`.
    template <class U>
    auto operator=(success<U> s) -> result& {
        m_v.template emplace<0>(std::move(s).value());
        return *this;
    }
    /// @brief Now holds the error of `f`.
    template <class G>
    auto operator=(failure<G> f) -> result& {
        m_v.template emplace<1>(std::move(f).error());
        return *this;
    }
    /// @brief Now holds a `T` built from `args`; returns it.
    template <class... Args>
    auto emplace(Args&&... args) -> T& {
        return m_v.template emplace<0>(std::forward<Args>(args)...);
    }
    ///@{
    /// @brief Exchanges contents with `other`.
    void swap(result& other) noexcept(std::is_nothrow_swappable_v<variant>) { m_v.swap(other.m_v); }
    friend void swap(result& a, result& b) noexcept(noexcept(a.swap(b))) { a.swap(b); }
    ///@}

    /// @brief True if it holds a value.
    constexpr auto has_value() const noexcept -> bool { return m_v.index() == 0; }
    /// @brief True if it holds a value.
    constexpr explicit operator bool() const noexcept { return has_value(); }

    ///@{
    /// @brief The value; only when there is one. Checked like `*` under `CP_CHECKS`.
    constexpr auto value() & -> T& { return at<0>(m_v); }
    constexpr auto value() const& -> T const& { return at<0>(m_v); }
    constexpr auto value() && -> T&& { return std::move(at<0>(m_v)); }
    constexpr auto value() const&& -> T const&& { return std::move(at<0>(m_v)); }
    constexpr auto operator*() & -> T& { return at<0>(m_v); }
    constexpr auto operator*() const& -> T const& { return at<0>(m_v); }
    constexpr auto operator*() && -> T&& { return std::move(at<0>(m_v)); }
    constexpr auto operator*() const&& -> T const&& { return std::move(at<0>(m_v)); }
    constexpr auto operator->() -> T* { return &at<0>(m_v); }
    constexpr auto operator->() const -> T const* { return &at<0>(m_v); }
    ///@}

    ///@{
    /// @brief The error; only when there is no value.
    constexpr auto error() & -> E& { return at<1>(m_v); }
    constexpr auto error() const& -> E const& { return at<1>(m_v); }
    constexpr auto error() && -> E&& { return std::move(at<1>(m_v)); }
    constexpr auto error() const&& -> E const&& { return std::move(at<1>(m_v)); }
    ///@}

    ///@{
    /// @brief The value, or `fallback` if there is none.
    template <class U>
    constexpr auto value_or(U&& fallback) const& -> T {
        return has_value() ? **this : static_cast<T>(std::forward<U>(fallback));
    }
    template <class U>
    constexpr auto value_or(U&& fallback) && -> T {
        return has_value() ? std::move(**this) : static_cast<T>(std::forward<U>(fallback));
    }
    ///@}

    /// @brief Equal if it holds a value equal to `v`.
    template <class U, std::enable_if_t<takes_as_value<U>, int> = 0>
    friend constexpr auto operator==(result const& a, U const& v) -> bool {
        return a.has_value() && static_cast<bool>(*a == v);
    }
#  if !defined(__cpp_impl_three_way_comparison)  // C++20 rewrites `!=` and the swapped `==` from the one above
    ///@{
    /// @brief Negated and swapped forms of the comparison above.
    template <class U, std::enable_if_t<takes_as_value<U>, int> = 0>
    friend constexpr auto operator==(U const& v, result const& a) -> bool {
        return a == v;
    }
    template <class U, std::enable_if_t<takes_as_value<U>, int> = 0>
    friend constexpr auto operator!=(result const& a, U const& v) -> bool {
        return !(a == v);
    }
    template <class U, std::enable_if_t<takes_as_value<U>, int> = 0>
    friend constexpr auto operator!=(U const& v, result const& a) -> bool {
        return !(a == v);
    }
    ///@}
#  endif

private:
    using variant = std::variant<T, E>;

    template <class R>
    static constexpr auto convert(R&& other) -> variant {
        if (other.has_value()) return variant(std::in_place_index<0>, *std::forward<R>(other));
        return variant(std::in_place_index<1>, std::forward<R>(other).error());
    }

    template <std::size_t I, class V>
    static constexpr auto at(V& v) -> decltype(auto) {
        detail::precondition(v.index() == I, I == 0 ? "result: no value" : "result: no error");
        return *std::get_if<I>(&v);
    }

    variant m_v;
};

/// @brief Success, or an error `E`.
template <class E>
class result<void, E> {
public:
    /// @brief Success.
    constexpr result() = default;
    /// @brief Success.
    constexpr result(success<void> /*s*/) noexcept { }  // NOLINT(*-explicit-*)
    /// @brief Success (as `std::expected<void, E>(std::in_place)`).
    constexpr explicit result(std::in_place_t /*tag*/) noexcept { }
    /// @brief Holds the error of `f`.
    template <class G>
    constexpr result(failure<G> f) : m_error(std::move(f).error()) { }  // NOLINT(*-explicit-*)
    ///@{
    /// @brief Success, or the error of `other` converted; explicit unless it converts implicitly, as `std::expected`.
    template <class G, std::enable_if_t<std::is_convertible_v<G const&, E>, int> = 0>
    constexpr result(result<void, G> const& other) : m_error(convert(other)) { }  // NOLINT(*-explicit-*)
    template <class G,
              std::enable_if_t<std::is_constructible_v<E, G const&> && !std::is_convertible_v<G const&, E>, int> = 0>
    constexpr explicit result(result<void, G> const& other) : m_error(convert(other)) { }
    template <class G, std::enable_if_t<std::is_convertible_v<G, E>, int> = 0>
    constexpr result(result<void, G>&& other) : m_error(convert(std::move(other))) { }  // NOLINT(*-explicit-*)
    template <class G, std::enable_if_t<std::is_constructible_v<E, G> && !std::is_convertible_v<G, E>, int> = 0>
    constexpr explicit result(result<void, G>&& other) : m_error(convert(std::move(other))) { }
    ///@}

    /// @brief Now success.
    auto operator=(success<void> /*s*/) noexcept -> result& {
        m_error.reset();
        return *this;
    }
    /// @brief Now holds the error of `f`.
    template <class G>
    auto operator=(failure<G> f) -> result& {
        m_error = std::move(f).error();
        return *this;
    }
    /// @brief Now success.
    void emplace() noexcept { m_error.reset(); }
    ///@{
    /// @brief Exchanges contents with `other`.
    void swap(result& other) noexcept(std::is_nothrow_swappable_v<E>) { m_error.swap(other.m_error); }
    friend void swap(result& a, result& b) noexcept(noexcept(a.swap(b))) { a.swap(b); }
    ///@}

    /// @brief True on success.
    constexpr auto has_value() const noexcept -> bool { return !m_error.has_value(); }
    /// @brief True on success.
    constexpr explicit operator bool() const noexcept { return has_value(); }
    ///@{
    /// @brief Nothing; only on success.
    constexpr void value() const { detail::precondition(has_value(), "result: no value"); }
    constexpr void operator*() const { value(); }
    ///@}
    ///@{
    /// @brief The error; only on failure.
    constexpr auto error() & -> E& {
        checked_error();
        return *m_error;
    }
    constexpr auto error() const& -> E const& {
        checked_error();
        return *m_error;
    }
    constexpr auto error() && -> E&& {
        checked_error();
        return std::move(*m_error);
    }
    ///@}

private:
    constexpr void checked_error() const { detail::precondition(m_error.has_value(), "result: no error"); }

    template <class R>
    static constexpr auto convert(R&& other) -> std::optional<E> {
        if (other.has_value()) return std::nullopt;
        return std::optional<E>(std::in_place, std::forward<R>(other).error());
    }

    std::optional<E> m_error;  // empty on success
};

/// @brief Equal if both hold equal values (or both succeeded, for `void`) or both hold equal errors.
template <class T, class E, class U, class G>
constexpr auto operator==(result<T, E> const& a, result<U, G> const& b) -> bool {
    if (a.has_value() != b.has_value()) return false;
    if (!a.has_value()) return static_cast<bool>(a.error() == b.error());
    if constexpr (std::is_void_v<T>) {
        return true;
    } else {
        return static_cast<bool>(*a == *b);
    }
}
/// @brief Equal if `r` holds an error equal to `f`'s.
template <class T, class E, class G>
constexpr auto operator==(result<T, E> const& r, failure<G> const& f) -> bool {
    return !r.has_value() && static_cast<bool>(r.error() == f.error());
}
#  if !defined(__cpp_impl_three_way_comparison)  // C++20 rewrites `!=` and the swapped `==` from the ones above
///@{
/// @brief Negated and swapped forms of the comparisons above.
template <class T, class E, class U, class G>
constexpr auto operator!=(result<T, E> const& a, result<U, G> const& b) -> bool {
    return !(a == b);
}
template <class T, class E, class G>
constexpr auto operator==(failure<G> const& f, result<T, E> const& r) -> bool {
    return r == f;
}
template <class T, class E, class G>
constexpr auto operator!=(result<T, E> const& r, failure<G> const& f) -> bool {
    return !(r == f);
}
template <class T, class E, class G>
constexpr auto operator!=(failure<G> const& f, result<T, E> const& r) -> bool {
    return !(r == f);
}
///@}
#  endif
#endif

}  // namespace cp::util

/// @brief Hash of a result: of its value, or of its error.
template <class T, class E>
struct std::hash<cp::util::result<T, E>> {
    /// @brief Hash of `r`.
    auto operator()(cp::util::result<T, E> const& r) const -> std::size_t {
        if (!r) return std::hash<E>{}(r.error());
        if constexpr (std::is_void_v<T>) {
            return 0;
        } else {
            return std::hash<T>{}(*r);
        }
    }
};
