#pragma once
/// @file
/// @brief cp::detail::located_arg: the first parameter of a variadic function that also records the caller's source
/// location (a defaulted parameter cannot follow a parameter pack).
#include "cp/config.hpp"
#include "cp/debug/source_location.hpp"
#include <memory>
#include <type_traits>

namespace cp::detail {

/// @brief Refers to one argument of any type and records the call's location. The location is a defaulted parameter
/// of the implicit converting constructor, so it is evaluated at the caller.
/// Calling it with `args...` runs `Op{}(args..., x)`, where `x` keeps the caller's value category (a variable is an
/// lvalue, a temporary an rvalue), so `Op` can reject temporaries with a `static_assert`, at the caller's line.
template <class Op, class... Args>
class located_arg {
public:
    /// @brief Refers to `x`; `loc` is the caller's location.
    // NOLINTNEXTLINE(google-explicit-constructor): converts implicitly, so the location is the caller's
    template <class T, std::enable_if_t<!std::is_same_v<std::decay_t<T>, located_arg>, int> = 0>
    located_arg(T&& x, debug::source_location loc = debug::source_location::current()) noexcept
        : m_ptr(static_cast<void const*>(std::addressof(x))), m_call(&m_invoke<T>), m_loc(loc) { }

    /// @brief Runs `Op{}(args..., x)`.
    void operator()(Args... args) const { m_call(m_ptr, args...); }
    /// @brief Where the call was made.
    auto location() const noexcept -> debug::source_location { return m_loc; }

private:
    template <class T>
    static void m_invoke(void const* p, Args... args) {
        using U = std::remove_reference_t<T>;
        Op{}(args..., static_cast<T&&>(*const_cast<U*>(static_cast<U const*>(p))));  // NOLINT(*-const-cast)
    }

    void const* m_ptr;
    void (*m_call)(void const*, Args...);
    debug::source_location m_loc;
};

}  // namespace cp::detail
