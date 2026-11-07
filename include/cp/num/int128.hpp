#pragma once
/// @file
/// @brief cp::num::i128 and cp::num::u128 (the compiler's `__int128`). Their `_i128` / `_u128` literals are in
/// cp/aliases/aliases.hpp.
/// @par Caveats
/// - Signed overflow is undefined, as for every built-in signed type.
/// - Printing and parsing come with cp's I/O module; standard streams cannot print `__int128`.

namespace cp::num {

/// @brief Signed 128-bit integer: the compiler's `__int128` (`__extension__` keeps `-pedantic` quiet).
__extension__ using i128 = __int128;
/// @brief Unsigned 128-bit integer.
__extension__ using u128 = unsigned __int128;

}  // namespace cp::num
