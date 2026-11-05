#pragma once
/// @file
/// @brief Detection idiom and C++20 `<type_traits>` backports.
#include "cp/compat/version.hpp"
#include <limits>
#include <type_traits>

/// @namespace cp
/// @brief Public API. Post-C++17 std facilities appear here as `std::x` or a same-interface backport.

/// @namespace cp::detail
/// @brief Implementation details, not public API. Holds testable `*_portable` / `*_backport` versions.

namespace cp {

/// @brief cp::detected_t result when the expression is ill-formed. Unusable by design.
struct nonesuch {
    nonesuch()                      = delete;
    ~nonesuch()                     = delete;
    nonesuch(nonesuch const&)       = delete;
    void operator=(nonesuch const&) = delete;
};

namespace detail {
/// @brief Integer types accepted by `<bit>`, `cmp_*` and saturation: integral, not `bool` or a character type.
template <class T>
inline constexpr bool is_integer_v =
    std::is_integral_v<T> && !std::is_same_v<T, bool> && !std::is_same_v<T, char> && !std::is_same_v<T, char16_t> &&
    !std::is_same_v<T, char32_t> && !std::is_same_v<T, wchar_t>
#if defined(__cpp_char8_t)  // cp:since c++20
    && !std::is_same_v<T, char8_t>
#endif
    ;

/// @brief Value bits of an arithmetic type: `std::numeric_limits<T>::digits`.
template <class T>
inline constexpr int digits_v = std::numeric_limits<T>::digits;
/// @brief Width in bits of an integer type, sign bit included.
template <class T>
inline constexpr int width_v = digits_v<T> + int{std::numeric_limits<T>::is_signed};
/// @brief Integers of any width, standard or not (`__int128`, cp's double-width types), except `bool`.
template <class T>
inline constexpr bool is_int_like_v = std::numeric_limits<T>::is_integer && !std::is_same_v<T, bool>;

/// @brief Detection idiom: `Op<Args...>` ill-formed.
template <class Default, class, template <class...> class Op, class... Args>
struct detector {
    using value_t = std::false_type;  ///< detection failed
    using type    = Default;          ///< fallback type
};
/// @brief Detection idiom: `Op<Args...>` well-formed.
template <class Default, template <class...> class Op, class... Args>
struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
    using value_t = std::true_type;  ///< detection succeeded
    using type    = Op<Args...>;     ///< the detected type
};
}  // namespace detail

/// @brief `std::true_type` if `Op<Args...>` is well-formed (Library Fundamentals TS v2).
/// @code
/// template <class T> using size_expr = decltype(std::declval<T&>().size());
/// static_assert(cp::is_detected_v<size_expr, std::vector<int>>);
/// @endcode
template <template <class...> class Op, class... Args>
using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

/// @brief `bool` value of cp::is_detected.
template <template <class...> class Op, class... Args>
inline constexpr bool is_detected_v = is_detected<Op, Args...>::value;

/// @brief `Op<Args...>` if well-formed, else cp::nonesuch.
template <template <class...> class Op, class... Args>
using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;

#if defined(__cpp_lib_remove_cvref)  // cp:since c++20
using std::remove_cvref;
using std::remove_cvref_t;
#else
/// @brief Backport of `std::remove_cvref`.
template <class T>
struct remove_cvref {
    using type = std::remove_cv_t<std::remove_reference_t<T>>;  ///< the stripped type
};
/// @brief Shorthand for `cp::remove_cvref<T>::type`.
template <class T>
using remove_cvref_t = typename remove_cvref<T>::type;
#endif

#if defined(__cpp_lib_type_identity)  // cp:since c++20
using std::type_identity;
using std::type_identity_t;
#else
/// @brief Backport of `std::type_identity`; blocks deduction: `f(T, cp::type_identity_t<T>)`.
template <class T>
struct type_identity {
    using type = T;  ///< `T` itself
};
/// @brief Shorthand for `cp::type_identity<T>::type`.
template <class T>
using type_identity_t = typename type_identity<T>::type;
#endif

#if defined(__cpp_lib_is_constant_evaluated)  // cp:since c++20
using std::is_constant_evaluated;
#else
/// @brief Backport of `std::is_constant_evaluated`.
/// @warning Always true inside `if constexpr` / `static_assert`; use a plain `if`.
constexpr auto is_constant_evaluated() noexcept { return __builtin_is_constant_evaluated(); }
#endif

}  // namespace cp
