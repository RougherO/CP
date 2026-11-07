#include "cp/ds/static_vector.hpp"
#include "lifetime.hpp"
#include <catch2/catch_template_test_macros.hpp>
#include <catch2/catch_test_macros.hpp>
#include <iterator>
#include <memory>
#include <new>
#include <sstream>
#include <tuple>
#include <type_traits>
#include <vector>
#if defined(__cpp_lib_ranges)
#  include <ranges>
#endif

namespace {

template <class T, std::size_t N>
using sv = cp::ds::static_vector<T, N>;  // the backport before C++26, std::inplace_vector from C++26

// One suite, run against the backport and (from C++26) std::inplace_vector.
template <template <class, std::size_t> class V>
struct family {
    template <class T, std::size_t N>
    using type = V<T, N>;
};
using families = std::tuple<family<sv>>;

template <template <class, std::size_t> class V>
void constructors() {
    V<int, 5> fill(3, 7);  // (n, value) is the fill constructor, not the iterator one
    REQUIRE(fill.size() == 3);
    REQUIRE(fill[2] == 7);
    std::vector<long> src{1, 2, 3};
    V<long, 5> from_iterators(src.begin(), src.end());
    REQUIRE(from_iterators.size() == 3);
    REQUIRE(from_iterators.back() == 3);
}

template <template <class, std::size_t> class V>
void capacity() {
    V<int, 2> full{1, 2};
    REQUIRE_THROWS_AS(full.push_back(3), std::bad_alloc);
    REQUIRE(full.size() == 2);
    REQUIRE_FALSE(full.try_push_back(3));
    full.pop_back();
    auto r = full.try_push_back(9);
    REQUIRE(r);
    REQUIRE(*r == 9);
    REQUIRE(&*r == &full.back());
    auto too_many = [] { return V<int, 2>(3); };
    REQUIRE_THROWS_AS(too_many(), std::bad_alloc);

    V<int, 2> two;
    REQUIRE_THROWS_AS(two.append_range(std::vector<int>{1, 2, 3}), std::bad_alloc);
    REQUIRE(two.empty());  // a forward range is checked before anything is appended
#if defined(__cpp_lib_ranges)
    V<int, 5> w;
    w.append_range(std::views::iota(0, 3L));  // end is a sentinel, not an iterator
    w.insert_range(w.begin(), std::views::iota(7, 8L));
    REQUIRE(w == V<int, 5>{7, 0, 1, 2});
#endif
}

template <template <class, std::size_t> class V>
void insert_erase() {
    V<int, 10> v{1, 2, 3};
    v.insert(v.begin(), v.back());  // the argument refers into the vector
    REQUIRE(v == V<int, 10>{3, 1, 2, 3});
    v.insert(v.begin() + 1, 2, 0);
    REQUIRE(v == V<int, 10>{3, 0, 0, 1, 2, 3});
    std::istringstream in("8 9");  // single-pass input
    v.insert(v.end() - 1, std::istream_iterator<int>(in), std::istream_iterator<int>());
    REQUIRE(v == V<int, 10>{3, 0, 0, 1, 2, 8, 9, 3});
    v.erase(v.begin() + 1, v.begin() + 3);
    REQUIRE(v == V<int, 10>{3, 1, 2, 8, 9, 3});
    REQUIRE(erase(v, 3) == 2);
    REQUIRE(erase_if(v, [](int x) { return x > 5; }) == 2);
    REQUIRE(v == V<int, 10>{1, 2});
    REQUIRE(V<int, 3>{1, 2} < V<int, 3>{1, 3});
}

template <template <class, std::size_t> class V>
void lifetime() {
    {
        V<tracked, 6> a{1, 2, 3, 4};
        V<tracked, 6> b{5};
        V<tracked, 6> c = a;
        b               = a;  // grow
        REQUIRE(b == a);
        REQUIRE(tracked::alive == 12);
        c = V<tracked, 6>{7};  // shrink
        REQUIRE(c.size() == 1);
        REQUIRE(c[0].v == 7);
        a.swap(c);
        REQUIRE(a.size() == 1);
        REQUIRE(c.size() == 4);
        REQUIRE(c[3].v == 4);
        swap(a, c);
        b.erase(b.begin() + 1);
        b.insert(b.begin(), tracked(0));
        b.resize(6);
        b.resize(2);
        V<tracked, 6> d = std::move(b);
        REQUIRE(d == V<tracked, 6>{0, 1});
        d.clear();
    }
    REQUIRE(tracked::alive == 0);  // every constructed element destroyed exactly once
}

}  // namespace

TEST_CASE("special members are trivial exactly when T's are", "[inplace_vector]") {
    STATIC_REQUIRE(std::is_trivially_copyable_v<sv<int, 4>>);
    STATIC_REQUIRE(!std::is_trivially_copyable_v<sv<tracked, 4>>);
    STATIC_REQUIRE(std::is_nothrow_move_constructible_v<sv<tracked, 4>>);
#if defined(__cpp_lib_inplace_vector) && __cpp_lib_inplace_vector >= 202603L
    STATIC_REQUIRE(std::is_same_v<cp::ds::static_vector<int, 3>, std::inplace_vector<int, 3>>);
#endif
}

TEMPLATE_LIST_TEST_CASE("inplace_vector constructors", "[inplace_vector]", families) {
    constructors<TestType::template type>();
}
TEMPLATE_LIST_TEST_CASE("inplace_vector capacity limits", "[inplace_vector]", families) {
    capacity<TestType::template type>();
}
TEMPLATE_LIST_TEST_CASE("inplace_vector insert and erase", "[inplace_vector]", families) {
    insert_erase<TestType::template type>();
}
TEMPLATE_LIST_TEST_CASE("inplace_vector element lifetimes", "[inplace_vector]", families) {
    lifetime<TestType::template type>();
}
