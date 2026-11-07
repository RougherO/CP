// cp::util::result: value or error, std::optional's interface, success and failure.
#include "cp/util/util.hpp"
#include <catch2/catch_test_macros.hpp>
#include <functional>
#include <memory>
#include <string>
#include <utility>

using cp::util::failure;
using cp::util::result;
using cp::util::success;

TEST_CASE("result holds a value or an error and compares like std::expected", "[util]") {
    result<std::string, int> a = std::string("ab"), b = failure(7);
    REQUIRE((a && *a == "ab" && a->size() == 2 && a.value() == "ab"));
    REQUIRE((!b && b.error() == 7 && b.value_or("x") == "x"));
    REQUIRE(a == std::string("ab"));
    REQUIRE(b == failure(7));
    REQUIRE(a != b);
    REQUIRE(result<std::string, int>{} == std::string());  // default: a value-initialised T

    b = "cd";  // assignment switches between value and error
    REQUIRE(b == std::string("cd"));
    a = failure(1);
    REQUIRE(a.error() == 1);
    swap(a, b);
    REQUIRE((a == std::string("cd") && b.error() == 1));
    result<long long, int> n = failure(1);
    REQUIRE((n.emplace(3) == 3 && n == 3LL));  // std::expected (C++23) emplaces only nothrow constructions
    REQUIRE(result<std::string, int>(std::in_place, 2, 'y') == std::string("yy"));

    result<long long, long> wide = result<int, short>(5);  // converts value and error types
    REQUIRE(wide == 5LL);
    result<int, std::unique_ptr<int>> moved = result<short, std::unique_ptr<int>>(failure(std::make_unique<int>(7)));
    REQUIRE(*moved.error() == 7);  // the move form takes move-only errors
    struct boxed {
        explicit boxed(int x) : v(x) { }
        int v;
    };
#if !defined(__cpp_lib_expected) || defined(__clang__)  // GCC 16 drops explicit(bool) on inherited constructors
    static_assert(!std::is_convertible_v<result<int, int>, result<boxed, int>>);  // explicit, as T's constructor
#endif
    REQUIRE(result<boxed, int>(result<int, int>(2))->v == 2);
    result<void, long> converted = result<void, int>(failure(8));
    REQUIRE((!converted && converted.error() == 8L && result<void, long>(result<void, int>())));
    REQUIRE(std::hash<result<int, int>>{}(failure(1)) == std::hash<result<int, int>>{}(failure(1)));  // equal results, equal hashes
}

TEST_CASE("result<void, E> is success or an error", "[util]") {
    result<void, int> ok, bad = failure(3);
    REQUIRE((ok && !bad && bad.error() == 3));
    REQUIRE(ok != bad);
    REQUIRE(bad == failure(3));
    bad.emplace();
    REQUIRE(bad == ok);
    bad = failure(4);
    REQUIRE(bad.error() == 4);
    bad = success();
    REQUIRE(bad);
}

TEST_CASE("success and failure build results like Rust's Ok and Err", "[util]") {
    auto half = [](int x) -> result<int, char> {
        if (x % 2 != 0) return failure('o');
        return success(x / 2);
    };
    REQUIRE(half(8) == 4);
    REQUIRE(half(7) == failure('o'));
    result<int, char> typed = failure<char>('t');  // explicit template arguments work too
    REQUIRE(typed.error() == 't');
}
