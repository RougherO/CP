#pragma once
/// @file
/// @brief cp::num::consts: the std::numbers constants (`pi`, `e`, `sqrt2`, …, and their `_v<T>` templates) plus
/// constants contests use: `tau`, `inf`, the usual moduli, the NTT root.
/// @par Caveats
/// - The floating-point `_v<T>` templates take floating-point types only, as std's do.
/// - Integer constants are `std::int64_t`, so `a * b % mod998` neither turns negative operands unsigned nor
///   overflows for `a, b < mod998`.
#include "cp/config.hpp"
#include <cstdint>
#include <limits>
#include <type_traits>
#if defined(__cpp_lib_math_constants)  // cp:since c++20
#  include <numbers>
#endif

namespace cp::detail {

/// @brief `T`, for floating-point `T` only: std::numbers' constraint on its `_v` templates.
template <class T>
using floating_only_t = std::enable_if_t<std::is_floating_point_v<T>, T>;

}  // namespace cp::detail

namespace cp::num::consts {

#if defined(__cpp_lib_math_constants)  // cp:since c++20
using std::numbers::e;
using std::numbers::e_v;
using std::numbers::egamma;
using std::numbers::egamma_v;
using std::numbers::inv_pi;
using std::numbers::inv_pi_v;
using std::numbers::inv_sqrt3;
using std::numbers::inv_sqrt3_v;
using std::numbers::inv_sqrtpi;
using std::numbers::inv_sqrtpi_v;
using std::numbers::ln10;
using std::numbers::ln10_v;
using std::numbers::ln2;
using std::numbers::ln2_v;
using std::numbers::log10e;
using std::numbers::log10e_v;
using std::numbers::log2e;
using std::numbers::log2e_v;
using std::numbers::phi;
using std::numbers::phi_v;
using std::numbers::pi;
using std::numbers::pi_v;
using std::numbers::sqrt2;
using std::numbers::sqrt2_v;
using std::numbers::sqrt3;
using std::numbers::sqrt3_v;
#else
// clang-format off
template <class T> inline constexpr T e_v          = static_cast<detail::floating_only_t<T>>(2.718281828459045235360287471352662498L);  ///< e
template <class T> inline constexpr T log2e_v      = static_cast<detail::floating_only_t<T>>(1.442695040888963407359924681001892137L);  ///< log₂ e
template <class T> inline constexpr T log10e_v     = static_cast<detail::floating_only_t<T>>(0.434294481903251827651128918916605082L);  ///< log₁₀ e
template <class T> inline constexpr T pi_v         = static_cast<detail::floating_only_t<T>>(3.141592653589793238462643383279502884L);  ///< π
template <class T> inline constexpr T inv_pi_v     = static_cast<detail::floating_only_t<T>>(0.318309886183790671537767526745028724L);  ///< 1/π
template <class T> inline constexpr T inv_sqrtpi_v = static_cast<detail::floating_only_t<T>>(0.564189583547756286948079451560772586L);  ///< 1/√π
template <class T> inline constexpr T ln2_v        = static_cast<detail::floating_only_t<T>>(0.693147180559945309417232121458176568L);  ///< ln 2
template <class T> inline constexpr T ln10_v       = static_cast<detail::floating_only_t<T>>(2.302585092994045684017991454684364208L);  ///< ln 10
template <class T> inline constexpr T sqrt2_v      = static_cast<detail::floating_only_t<T>>(1.414213562373095048801688724209698079L);  ///< √2
template <class T> inline constexpr T sqrt3_v      = static_cast<detail::floating_only_t<T>>(1.732050807568877293527446341505872367L);  ///< √3
template <class T> inline constexpr T inv_sqrt3_v  = static_cast<detail::floating_only_t<T>>(0.577350269189625764509148780501957456L);  ///< 1/√3
template <class T> inline constexpr T egamma_v     = static_cast<detail::floating_only_t<T>>(0.577215664901532860606512090082402431L);  ///< Euler–Mascheroni γ
template <class T> inline constexpr T phi_v        = static_cast<detail::floating_only_t<T>>(1.618033988749894848204586834365638118L);  ///< golden ratio φ

inline constexpr double e          = e_v<double>;           ///< e
inline constexpr double log2e      = log2e_v<double>;       ///< log₂ e
inline constexpr double log10e     = log10e_v<double>;      ///< log₁₀ e
inline constexpr double pi         = pi_v<double>;          ///< π
inline constexpr double inv_pi     = inv_pi_v<double>;      ///< 1/π
inline constexpr double inv_sqrtpi = inv_sqrtpi_v<double>;  ///< 1/√π
inline constexpr double ln2        = ln2_v<double>;         ///< ln 2
inline constexpr double ln10       = ln10_v<double>;        ///< ln 10
inline constexpr double sqrt2      = sqrt2_v<double>;       ///< √2
inline constexpr double sqrt3      = sqrt3_v<double>;       ///< √3
inline constexpr double inv_sqrt3  = inv_sqrt3_v<double>;   ///< 1/√3
inline constexpr double egamma     = egamma_v<double>;      ///< Euler–Mascheroni γ
inline constexpr double phi        = phi_v<double>;         ///< golden ratio φ
// clang-format on
#endif

/// @brief 2π, a full turn in radians.
template <class T>
inline constexpr T tau_v = static_cast<detail::floating_only_t<T>>(6.283185307179586476925286766559005768L);
/// @brief 2π as `double`.
inline constexpr double tau = tau_v<double>;

/// @brief A large value for "unreached": integers: `max() / 2`, so `inf + inf` does not overflow; floating point:
/// infinity.
template <class T>
inline constexpr T inf_v = std::numeric_limits<T>::has_infinity ? std::numeric_limits<T>::infinity()
                                                                : static_cast<T>(std::numeric_limits<T>::max() / 2);
/// @brief inf_v for `std::int64_t`: 4611686018427387903.
inline constexpr std::int64_t inf = inf_v<std::int64_t>;

inline constexpr std::int64_t mod998     = 998'244'353;                  ///< 998244353 = 119·2²³ + 1, the NTT prime
inline constexpr std::int64_t mod1e9_7   = 1'000'000'007;                ///< 10⁹ + 7, prime
inline constexpr std::int64_t mod1e9_9   = 1'000'000'009;                ///< 10⁹ + 9, prime
inline constexpr std::int64_t mersenne61 = (std::int64_t{1} << 61) - 1;  ///< 2⁶¹ − 1, prime: rolling hashes
inline constexpr std::int64_t ntt_root   = 3;                            ///< a primitive root modulo mod998

}  // namespace cp::num::consts
