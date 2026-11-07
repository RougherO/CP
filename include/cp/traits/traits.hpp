#pragma once
/// @file
/// @brief cp::traits: the detection idiom, type traits and compile-time helpers (`remove_cvref_t`, `type_identity_t`,
/// `is_constant_evaluated`, `to_underlying`), the std versions when the standard has them. Also the traits the rest
/// of cp shares internally (cp::detail); range and iterator helpers are in cp/detail/range.hpp.
#include "cp/config.hpp"
#include <limits>
#include <type_traits>
#if defined(__cpp_lib_to_underlying)  // cp:since c++23
#  include <utility>
#endif

/// @namespace cp
/// @brief The library. Each module is a namespace: cp::num, cp::bits, cp::ds, cp::views, cp::traits, cp::debug,
/// cp::sys, cp::io, cp::util, cp::aliases.

/// @namespace cp::detail
/// @brief Implementation details shared by all modules, not public API.

namespace cp::detail {

/// @brief True for the character types (`char`, `wchar_t`, `char8_t`, `char16_t`, `char32_t`); `signed char` and
/// `unsigned char` are integers.
template <class T>
inline constexpr bool is_char_v = std::is_same_v<T, char> || std::is_same_v<T, wchar_t> ||
#if defined(__cpp_char8_t)  // cp:since c++20
                                  std::is_same_v<T, char8_t> ||
#endif
                                  std::is_same_v<T, char16_t> || std::is_same_v<T, char32_t>;

/// @brief The integers cp accepts everywhere: any width, standard or not (`__int128`, cp's double-width types), but
/// not `bool` or a character type.
template <class T>
inline constexpr bool is_integer_v = std::numeric_limits<T>::is_integer && !std::is_same_v<T, bool> && !is_char_v<T>;

/// @brief Value bits of an arithmetic type: `std::numeric_limits<T>::digits`.
template <class T>
inline constexpr int digits_v = std::numeric_limits<T>::digits;
/// @brief Width in bits of an integer type, sign bit included.
template <class T>
inline constexpr int width_v = digits_v<T> + int{std::numeric_limits<T>::is_signed};

/// @brief The unsigned type of the same width: `std::make_unsigned` (which covers `__int128`); cp's double-width
/// integers specialise it in cp/detail/double_int.hpp.
template <class T>
struct unsigned_of : std::make_unsigned<T> { };
/// @brief `unsigned_of<T>::type`.
template <class T>
using unsigned_of_t = typename unsigned_of<T>::type;

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

}  // namespace cp::detail

/// @namespace cp::traits
/// @brief Type traits and the detection idiom.
namespace cp::traits {

/// @brief traits::detected_t result when the expression is ill-formed. Unusable by design.
struct nonesuch {
    nonesuch()                      = delete;
    ~nonesuch()                     = delete;
    nonesuch(nonesuch const&)       = delete;
    void operator=(nonesuch const&) = delete;
};

/// @brief `std::true_type` if `Op<Args...>` is well-formed (Library Fundamentals TS v2).
/// @code
/// template <class T> using size_expr = decltype(std::declval<T&>().size());
/// static_assert(traits::is_detected_v<size_expr, std::vector<int>>);
/// @endcode
template <template <class...> class Op, class... Args>
using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

/// @brief `bool` value of traits::is_detected.
template <template <class...> class Op, class... Args>
inline constexpr bool is_detected_v = is_detected<Op, Args...>::value;

/// @brief `Op<Args...>` if well-formed, else traits::nonesuch.
template <template <class...> class Op, class... Args>
using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;

///@{
/// @brief True if `T` is `Template<Args...>` for some `Args` (type parameters only):
/// `is_specialization_of_v<std::vector<int>, std::vector>`.
template <class T, template <class...> class Template>
inline constexpr bool is_specialization_of_v = false;
template <template <class...> class Template, class... Args>
inline constexpr bool is_specialization_of_v<Template<Args...>, Template> = true;
///@}

#if defined(__cpp_lib_remove_cvref)  // cp:since c++20
using std::remove_cvref;
using std::remove_cvref_t;
#else
/// @brief Backport of `std::remove_cvref`.
template <class T>
struct remove_cvref {
    using type = std::remove_cv_t<std::remove_reference_t<T>>;  ///< the stripped type
};
/// @brief Shorthand for `traits::remove_cvref<T>::type`.
template <class T>
using remove_cvref_t = typename remove_cvref<T>::type;
#endif

#if defined(__cpp_lib_type_identity)  // cp:since c++20
using std::type_identity;
using std::type_identity_t;
#else
/// @brief Backport of `std::type_identity`; blocks deduction: `f(T, traits::type_identity_t<T>)`.
template <class T>
struct type_identity {
    using type = T;  ///< `T` itself
};
/// @brief Shorthand for `traits::type_identity<T>::type`.
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

#if defined(__cpp_lib_to_underlying)  // cp:since c++23
using std::to_underlying;
#else
/// @brief Converts an enum to its underlying integer type.
template <class E>
constexpr auto to_underlying(E e) noexcept {
    return static_cast<std::underlying_type_t<E>>(e);
}
#endif

}  // namespace cp::traits
