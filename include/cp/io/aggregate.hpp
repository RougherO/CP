#pragma once
/// @file
/// @brief Fields of plain structs (aggregates) without user code: cp::detail::field_count_v,
/// cp::detail::with_fields and cp::detail::for_each_field. Used by cp's I/O to read and write aggregates automatically.
/// @par Caveats
/// - Up to C++23: the field count comes from brace-initialization probing (the Boost.PFR technique) and fields are
///   reached through structured bindings, for up to 32 fields. Not supported: base classes, C-array members (brace
///   elision miscounts them; use `std::array`), and members that are themselves constructible from anything (e.g.
///   `std::optional`, `std::any`).
/// - C++26 with reflection (`__cpp_impl_reflection`, GCC 16 `-freflection`): fields come from
///   `std::meta::nonstatic_data_members_of`, with no field limit and no C-array restriction.
/// - with_fields has one mechanical case per field count: case n binds n fields.
#include "cp/config.hpp"
#include <cstddef>
#include <type_traits>
#include <utility>
#if defined(__cpp_impl_reflection)  // cp:since c++26
#  include <meta>
#endif

namespace cp::detail {

/// @brief Most fields an aggregate may have for cp's automatic I/O (C++17–23).
inline constexpr std::size_t max_fields = 32;

#if defined(__cpp_impl_reflection)  // cp:since c++26

/// @brief The field count of `T`.
template <class T>
inline constexpr std::size_t field_count_v =
    std::meta::nonstatic_data_members_of(^^std::remove_cv_t<T>, std::meta::access_context::unchecked()).size();

/// @brief `fn(fields...)`: every field of `t` at once, in declaration order. Immediate (splicing escalates it), so
/// only for unevaluated use, such as the fields' types.
template <class T, class F>
constexpr auto with_fields(T& t, F&& fn) -> decltype(auto) {
    constexpr auto members = std::define_static_array(
        std::meta::nonstatic_data_members_of(^^std::remove_cv_t<T>, std::meta::access_context::unchecked()));
    return [&]<std::size_t... I>(std::index_sequence<I...>) -> decltype(auto) {
        return fn(t.[:members[I]:]...);
    }(std::make_index_sequence<members.size()>{});
}

/// @brief Calls `fn(field)` for each field of `t`, in declaration order.
template <class T, class F>
constexpr void for_each_field(T& t, F&& fn) {
    template for (constexpr auto m : std::define_static_array(std::meta::nonstatic_data_members_of(
                      ^^std::remove_cv_t<T>, std::meta::access_context::unchecked()))) fn(t.[:m:]);
}

#else

/// @brief Converts to anything; only ever used unevaluated, to count an aggregate's fields.
struct any_field {
    /// @brief Declared only.
    template <class T>
    operator T() const;  // NOLINT(google-explicit-constructor): implicit by design
};

template <class T, class Seq, class = void>
struct brace_constructible_n : std::false_type { };
template <class T, std::size_t... I>
struct brace_constructible_n<T, std::index_sequence<I...>, std::void_t<decltype(T{(void(I), any_field{})...})>>
    : std::true_type { };

/// @brief The largest N for which `T{x1, …, xN}` compiles: the field count of an aggregate without bases.
template <class T, std::size_t N = max_fields + 1>
constexpr auto count_fields() noexcept -> std::size_t {
    if constexpr (N == 0 || brace_constructible_n<T, std::make_index_sequence<N>>::value)
        return N;
    else
        return count_fields<T, N - 1>();
}

/// @brief The field count of `T`.
template <class T>
inline constexpr std::size_t field_count_v = count_fields<std::remove_cv_t<T>>();

/// @brief `fn(fields...)`: every field of `t` at once, in declaration order.
template <class T, class F>
constexpr auto with_fields(T& t, F&& fn) -> decltype(auto) {
    constexpr std::size_t n = field_count_v<T>;
    static_assert(n <= max_fields, "cp: automatic I/O supports aggregates of at most 32 fields; add cp_read/cp_write");
    if constexpr (n == 0) {
        static_cast<void>(t);
        return fn();
    } else if constexpr (n == 1) {
        auto& [a] = t;
        return fn(a);
    } else if constexpr (n == 2) {
        auto& [a, b] = t;
        return fn(a, b);
    } else if constexpr (n == 3) {
        auto& [a, b, c] = t;
        return fn(a, b, c);
    } else if constexpr (n == 4) {
        auto& [a, b, c, d] = t;
        return fn(a, b, c, d);
    } else if constexpr (n == 5) {
        auto& [a, b, c, d, e] = t;
        return fn(a, b, c, d, e);
    } else if constexpr (n == 6) {
        auto& [a, b, c, d, e, f] = t;
        return fn(a, b, c, d, e, f);
    } else if constexpr (n == 7) {
        auto& [a, b, c, d, e, f, g] = t;
        return fn(a, b, c, d, e, f, g);
    } else if constexpr (n == 8) {
        auto& [a, b, c, d, e, f, g, h] = t;
        return fn(a, b, c, d, e, f, g, h);
    } else if constexpr (n == 9) {
        auto& [a, b, c, d, e, f, g, h, i] = t;
        return fn(a, b, c, d, e, f, g, h, i);
    } else if constexpr (n == 10) {
        auto& [a, b, c, d, e, f, g, h, i, j] = t;
        return fn(a, b, c, d, e, f, g, h, i, j);
    } else if constexpr (n == 11) {
        auto& [a, b, c, d, e, f, g, h, i, j, k] = t;
        return fn(a, b, c, d, e, f, g, h, i, j, k);
    } else if constexpr (n == 12) {
        auto& [a, b, c, d, e, f, g, h, i, j, k, l] = t;
        return fn(a, b, c, d, e, f, g, h, i, j, k, l);
    } else if constexpr (n == 13) {
        auto& [a, b, c, d, e, f, g, h, i, j, k, l, m] = t;
        return fn(a, b, c, d, e, f, g, h, i, j, k, l, m);
    } else if constexpr (n == 14) {
        auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, o] = t;
        return fn(a, b, c, d, e, f, g, h, i, j, k, l, m, o);
    } else if constexpr (n == 15) {
        auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, o, p] = t;
        return fn(a, b, c, d, e, f, g, h, i, j, k, l, m, o, p);
    } else if constexpr (n == 16) {
        auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q] = t;
        return fn(a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q);
    } else if constexpr (n == 17) {
        auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r] = t;
        return fn(a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r);
    } else if constexpr (n == 18) {
        auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s] = t;
        return fn(a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s);
    } else if constexpr (n == 19) {
        auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u] = t;
        return fn(a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u);
    } else if constexpr (n == 20) {
        auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v] = t;
        return fn(a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v);
    } else if constexpr (n == 21) {
        auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w] = t;
        return fn(a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w);
    } else if constexpr (n == 22) {
        auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w, x] = t;
        return fn(a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w, x);
    } else if constexpr (n == 23) {
        auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w, x, y] = t;
        return fn(a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w, x, y);
    } else if constexpr (n == 24) {
        auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w, x, y, z] = t;
        return fn(a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w, x, y, z);
    } else if constexpr (n == 25) {
        auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w, x, y, z, za] = t;
        return fn(a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w, x, y, z, za);
    } else if constexpr (n == 26) {
        auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w, x, y, z, za, zb] = t;
        return fn(a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w, x, y, z, za, zb);
    } else if constexpr (n == 27) {
        auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w, x, y, z, za, zb, zc] = t;
        return fn(a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w, x, y, z, za, zb, zc);
    } else if constexpr (n == 28) {
        auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w, x, y, z, za, zb, zc, zd] = t;
        return fn(a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w, x, y, z, za, zb, zc, zd);
    } else if constexpr (n == 29) {
        auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w, x, y, z, za, zb, zc, zd, ze] = t;
        return fn(a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w, x, y, z, za, zb, zc, zd, ze);
    } else if constexpr (n == 30) {
        auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w, x, y, z, za, zb, zc, zd, ze, zf] = t;
        return fn(a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w, x, y, z, za, zb, zc, zd, ze, zf);
    } else if constexpr (n == 31) {
        auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w, x, y, z, za, zb, zc, zd, ze, zf, zg] = t;
        return fn(a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w, x, y, z, za, zb, zc, zd, ze, zf, zg);
    } else if constexpr (n == 32) {
        auto& [a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w, x, y, z, za, zb, zc, zd, ze, zf, zg, zh] =
            t;
        return fn(a, b, c, d, e, f, g, h, i, j, k, l, m, o, p, q, r, s, u, v, w, x, y, z, za, zb, zc, zd, ze, zf, zg,
                  zh);
    }
}

/// @brief Calls `fn(field)` for each field of `t`, in declaration order.
template <class T, class F>
constexpr void for_each_field(T& t, F&& fn) {
    with_fields(t, [&fn](auto&... fields) { (fn(fields), ...); });
}

#endif

}  // namespace cp::detail
