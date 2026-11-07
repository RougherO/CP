#pragma once
/// @file
/// @brief Every module in cp library
#include "cp/config.hpp"

// IWYU pragma: begin_exports
#include "cp/aliases/aliases.hpp"
#include "cp/bits/bits.hpp"
#include "cp/debug/debug.hpp"
#include "cp/ds/ds.hpp"
#include "cp/io/io.hpp"
#include "cp/num/num.hpp"
#include "cp/sys/sys.hpp"
#include "cp/traits/traits.hpp"
#include "cp/util/util.hpp"
#include "cp/views/views.hpp"
// IWYU pragma: end_exports

using namespace cp;
using namespace std;
using namespace aliases;

#define CP_MAIN(is_test_case_based)                   \
    int main() {                                      \
        if constexpr (is_test_case_based) {           \
            for (auto t = *io::read<usize>(); t--;) { \
                solve(t);                             \
            }                                         \
        } else {                                      \
            solve(0);                                 \
        }                                             \
    }
