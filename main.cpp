#include <bits/stdc++.h>
#ifdef __cpp_lib_ranges
namespace r = std::ranges;
namespace v = std::views;
#endif
/**
 * Made with ❤️
 *
 * - C++17 compatible
 * - Easy and convenient
 *
 * For always updated template: https://github.com/RougherO/CP.git
 * Follow me: https://github.com/RougherO
 *
 * If you decide to use the template please attach this section
 * as well or mention the source as a token of appreciation.
 *
 * Thanks
 */
namespace speed {
namespace ds {
    struct mint;
    struct dsu;
}
namespace str {
    auto split(std::string_view, std::string_view = "") -> std::vector<std::string_view>;
    auto strip(std::string_view) -> std::string_view;
}
namespace utils {
    template <typename>
    struct is_tuple_like : std::false_type { };
    template <typename... Ts>
    struct is_tuple_like<std::tuple<Ts...>> : std::true_type { };
    template <typename T1, typename T2>
    struct is_tuple_like<std::pair<T1, T2>> : std::true_type { };
    template <typename T, std::size_t N>
    struct is_tuple_like<std::array<T, N>> : std::true_type { };
    template <typename T>
    constexpr bool is_tuple_like_v = is_tuple_like<T>::value;
    template <typename T, typename = void>
    struct is_iterator : std::false_type { };
    template <typename T>
    struct is_iterator<T, std::void_t<typename std::iterator_traits<T>::iterator_category>> : std::true_type { };
    template <typename T>
    constexpr bool is_iterator_v = is_iterator<T>::value;
    template <typename T>
    struct is_integer : std::disjunction<std::is_integral<T>, std::is_same<T, ds::mint>> { };
    template <typename T>
    constexpr bool is_integer_v = is_integer<T>::value;
}
namespace math {
    using utils::is_integer_v;
    template <typename T, typename = std::enable_if_t<is_integer_v<T>>>
    constexpr auto binary_expo(T, unsigned long long) -> T;
    template <typename T, typename = std::enable_if_t<is_integer_v<T>>>
    constexpr auto binomial_coeff(T, T) -> T;
    template <typename T, typename = std::enable_if_t<is_integer_v<T>>> // use tgamma for floating points
    constexpr auto factorial(T) noexcept -> T;
    template <typename T, typename = std::enable_if_t<is_integer_v<T>>>
    constexpr auto is_prime(T) -> bool;
    template <typename T, typename = std::enable_if_t<is_integer_v<T>>>
    constexpr auto floor(T, T) -> T;
    template <typename T, typename = std::enable_if_t<is_integer_v<T>>>
    constexpr auto ceil(T, T) -> T;
    template <typename T, typename = std::enable_if_t<is_integer_v<T>>>
    auto divisors(T) -> std::vector<T>;
    template <typename T>
    constexpr auto nmax() { return std::numeric_limits<T>::max(); }
    template <typename T>
    constexpr auto nmin() { return std::numeric_limits<T>::min(); }
}
namespace io {
    static int const pretty_index = std::ios_base::xalloc(); // For pretty printing
    using utils::is_tuple_like_v;
    auto operator>>(std::istream&, ds::mint&) -> std::istream&;
    template <typename... Params>
    auto operator>>(std::istream&, std::vector<Params...>&) -> std::istream&;
    template <typename Type, typename = std::enable_if_t<is_tuple_like_v<Type>>>
    auto operator>>(std::istream&, Type&) -> std::istream&;
    template <typename... Ts>
    void scan(std::istream&, Ts&...);
    template <typename... Ts>
    void scan(Ts&... args) { scan(std::cin, args...); }
    void scanln(std::istream&, std::string&);
    void scanln(std::string& line) { scanln(std::cin, line); }
    using utils::is_iterator_v;
    template <typename T, std::enable_if_t<is_iterator_v<T>>>
    auto operator<<(std::ostream&, std::pair<T, T>) -> std::ostream&;
    auto operator<<(std::ostream&, std::string_view) -> std::ostream&;
    auto operator<<(std::ostream&, std::string const&) -> std::ostream&;
    template <typename Type, typename = std::enable_if_t<is_tuple_like_v<Type>>>
    auto operator<<(std::ostream&, Type& tuple) -> std::ostream&;
    template <typename... Params>
    auto operator<<(std::ostream&, std::vector<Params...> const&) -> std::ostream&;
    template <typename... Params>
    auto operator<<(std::ostream&, std::set<Params...> const&) -> std::ostream&;
    template <typename... Params>
    auto operator<<(std::ostream&, std::unordered_set<Params...> const&) -> std::ostream&;
    template <typename... Params>
    auto operator<<(std::ostream&, std::map<Params...> const&) -> std::ostream&;
    template <typename... Params>
    auto operator<<(std::ostream&, std::unordered_map<Params...> const&) -> std::ostream&;
    template <typename T, std::size_t N>
    auto operator<<(std::ostream&, std::array<T, N> const&) -> std::ostream&;
    template <typename... Ts>
    auto operator<<(std::ostream&, std::tuple<Ts...> const&) -> std::ostream&;
    template <typename T, typename U>
    auto operator<<(std::ostream&, std::pair<T, U> const&) -> std::ostream&;
    auto operator<<(std::ostream&, ds::mint const&) -> std::ostream&;
    template <char... Seps, typename T, typename = std::enable_if_t<is_iterator_v<T>>>
    void put(std::ostream&, T, T);
    template <char... Seps, typename T, typename = std::enable_if_t<is_iterator_v<T>>>
    void put(T first, T last) { put<Seps...>(std::cout, first, last); }
    template <char... Seps, typename... Ts>
    void put(std::ostream& os, Ts const&... args);
    template <char... Seps, typename... Ts>
    void put(Ts const&... args) { put<Seps...>(std::cout, args...); }
    template <char... Seps, typename T, typename = std::enable_if_t<is_iterator_v<T>>>
    void putln(std::ostream& os, T first, T last)
    {
        put<Seps...>(os, first, last);
        os << "\n";
    }
    template <char... Seps, typename T, typename = std::enable_if_t<is_iterator_v<T>>>
    void putln(T first, T last) { putln<Seps...>(std::cout, first, last); }
    template <char... Seps, typename... Ts>
    void putln(std::ostream& os, Ts const&... args)
    {
        put<Seps...>(os, args...);
        os << "\n";
    }
    template <char... Seps, typename... Ts>
    void putln(Ts const&... args) { putln<Seps...>(std::cout, args...); }
    auto pretty(std::ostream& os) -> std::ostream&
    {
        os.iword(pretty_index) = !os.iword(pretty_index); // set pretty printing to
        return os;
    }
    template <typename... Ts>
    void debug(char const* name_str, Ts const&... args);
}
namespace utils {
    using io::operator<<;
    using io::operator>>;
    template <typename, typename = void>
    struct is_scannable : std::false_type { };
    template <typename T>
    struct is_scannable<T, std::void_t<decltype(std::declval<std::istream&>() >> std::declval<std::decay_t<T>&>())>> : std::true_type { };
    template <typename T>
    constexpr bool is_scannable_v = is_scannable<T>::value;
    template <typename, typename = void>
    struct is_printable : std::false_type { };
    template <typename T>
    struct is_printable<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<std::decay_t<T>&>())>> : std::true_type { };
    template <typename T>
    constexpr bool is_printable_v = is_printable<T>::value;
}
namespace ds {
    struct mint {
        constexpr mint() = default;
        constexpr mint(long long value)
            : x { value % mod }
        {
        }
        constexpr mint(long long value, long long modulus)
            : mod { modulus }
            , x { value % modulus }
        {
        }
        constexpr auto operator+(mint const& o) const noexcept -> mint { return (x + o.x) % mod; }
        constexpr auto operator-(mint const& o) const noexcept -> mint { return (x - o.x) % mod + (x < o.x ? mod : 0); }
        constexpr auto operator*(mint const& o) const noexcept -> mint { return (x * o.x) % mod; }
        constexpr auto operator/(mint const& o) const noexcept -> mint { return mint { x } * math::binary_expo(o, mod - 2); }
        constexpr auto operator%(mint const& o) const noexcept -> mint { return x % o.x; }
        constexpr auto operator<<(mint const& o) const noexcept -> mint { return (x << o.x) % mod; }
        constexpr auto operator>>(mint const& o) const noexcept -> mint { return (x >> o.x) % mod; }
        constexpr auto operator|(mint const& o) const noexcept -> mint { return x | o.x; }
        constexpr auto operator&(mint const& o) const noexcept -> mint { return x & o.x; }
        constexpr auto operator~() const noexcept -> mint { return ~x % mod; }
        constexpr auto operator+=(mint const& o) noexcept -> mint& { return *this = *this + o; }
        constexpr auto operator-=(mint const& o) noexcept -> mint& { return *this = *this - o; }
        constexpr auto operator*=(mint const& o) noexcept -> mint& { return *this = *this * o; }
        constexpr auto operator/=(mint const& o) noexcept -> mint& { return *this = *this / o; }
        constexpr auto operator%=(mint const& o) noexcept -> mint& { return *this = *this % o; }
        constexpr auto operator<<=(mint const& o) noexcept -> mint { return *this = *this << o; }
        constexpr auto operator>>=(mint const& o) noexcept -> mint { return *this = *this >> o; }
        constexpr auto operator|=(mint const& o) noexcept -> mint { return *this = *this | o; }
        constexpr auto operator&=(mint const& o) noexcept -> mint { return *this = *this & o; }
        constexpr auto operator++() noexcept -> mint& { return *this += 1; }
        constexpr auto operator++(int) noexcept -> mint
        {
            auto temp  = *this;
            *this     += 1;
            return temp;
        }
        constexpr auto operator==(mint const& other) const noexcept -> bool { return x == other.x; };
        constexpr auto operator!=(mint const& other) const noexcept -> bool { return !(*this == other); }
        constexpr auto operator<(mint const& other) const noexcept -> bool { return x < other.x; }
        constexpr auto operator>(mint const& other) const noexcept -> bool { return !(*this < other) && !(*this == other); }
        constexpr auto operator<=(mint const& other) const noexcept -> bool { return !(*this > other); }
        constexpr auto operator>=(mint const& other) const noexcept -> bool { return !(*this < other); }
        constexpr auto operator!() const noexcept -> bool { return !x; }
        constexpr explicit operator int() const noexcept { return x; }
        constexpr auto value() const noexcept -> long long { return x; }

    private:
        long long mod { 1000000007 };
        long long x {};
    };
    struct dsu {
        dsu(size_t n)
            : m_rank(n)
            , m_parent(n)
        {
            std::iota(m_parent.begin(), m_parent.end(), 0);
        }
        auto parent(size_t node) noexcept -> size_t
        {
            if (m_parent[node] == node) {
                return node;
            }
            return m_parent[node] = parent(m_parent[node]);
        }
        auto unite(size_t node1, size_t node2) noexcept -> bool
        {
            size_t p1 = parent(node1);
            size_t p2 = parent(node2);
            if (p1 == p2) {
                return false;
            }
            if (m_rank[p1] > m_rank[p2]) {
                m_parent[p2] = p1;
            } else {
                m_parent[p1] = p2;
                if (m_rank[p1] == m_rank[p2]) {
                    m_rank[p2]++;
                }
            }
            return true;
        }
        auto count() const noexcept -> size_t
        {
            size_t n = m_parent.size();
            size_t cnt {};
            for (size_t i = 0; i != n; ++i) {
                if (i == m_parent[i]) {
                    cnt++;
                }
            }
            return cnt;
        }

    private:
        std::vector<size_t> m_rank;
        std::vector<size_t> m_parent;
    };
}
namespace str {
    auto split(std::string_view line, std::string_view delim) -> std::vector<std::string_view>
    {
        std::vector<std::string_view> cont;
        size_t s = 0;
        size_t e = 0;
        if (delim.size() == 0) {
            size_t size = line.size();
            while (e < size) {
                s = e;
                while (s < size && std::isspace(line[s])) {
                    s++;
                }
                if (s == size) {
                    break;
                }
                e = s;
                while (e < size && !std::isspace(line[e])) {
                    e++;
                }
                cont.emplace_back(line.substr(s, e - s));
            }
        } else {
            while ((e = line.find(delim, s)) != std::string::npos) {
                cont.emplace_back(line.substr(s, e - s));
                s = e + delim.size();
            }
            cont.emplace_back(line.substr(s));
        }
        return cont;
    }
    auto strip(std::string_view word) -> std::string_view
    {
        size_t l = word.find_first_not_of(' ');
        size_t r = word.find_last_not_of(' ');
        return word.substr(l, r - l + 1);
    }
}
namespace math {
    template <typename T, typename>
    constexpr auto binary_expo(T base, unsigned long long pow) -> T
    {
        T result { 1 };
        while (pow != 0) {
            if (pow % 2 == 1) {
                result *= base;
            }
            base *= base;
            pow  /= 2;
        }
        return result;
    };
    template <typename T, typename>
    constexpr auto binomial_coeff(T n, T r) -> T
    {
        if (r > n - r) {
            r = n - r;
        }
        T prod { 1 };
        for (T i = 0; i != r; i++) {
            prod *= (n - i);
            prod /= i + 1;
        }
        return prod;
    }
    template <typename T, typename>
    constexpr auto factorial(T n) noexcept -> T
    {
        T prod { 1 };
        for (T i = 2; i <= n; i++) {
            prod *= i;
        }
        return prod;
    }
    template <typename T, typename>
    constexpr auto is_prime(T n) -> bool
    {
        switch (n) {
        case 0:
        case 1:
            return false;
        case 2:
        case 3:
            return true;
        }
        if (n % 2 == 0 || n % 3 == 0) {
            return false;
        }
        for (T i { 5 }; i * i <= n; i += 2) {
            if (n % i == 0) {
                return false;
            }
        }
        return true;
    }
    template <typename T, typename>
    constexpr auto floor(T x, T y) -> T
    {
        return x / y;
    }
    template <typename T, typename>
    constexpr auto ceil(T x, T y) -> T
    {
        return (x + y - 1) / y;
    }
    template <typename T, typename>
    auto divisors(T n) -> std::vector<T>
    {
        T x { 1 };
        std::vector<T> divs;
        while (x * x <= n) {
            if (n % x == 0) {
                divs.insert(divs.end(), { x, n / x });
            }
            x++;
        }
        return divs;
    }
}
namespace io {
    auto operator>>(std::istream& is, ds::mint& m) -> std::istream&
    {
        long long x {};
        is >> x;
        m = x;
        return is;
    }
    template <typename... Params>
    auto operator>>(std::istream& is, std::vector<Params...>& v) -> std::istream&
    {
        for (auto& e : v) {
            is >> e;
        }
        return is;
    }
    template <typename TupleType, typename>
    auto operator>>(std::istream& is, TupleType& t) -> std::istream&
    {
        return std::apply([&is](auto&... args) -> std::istream& { return (is >> ... >> args); }, t);
    }
    namespace detail {
        template <typename Type, typename = std::enable_if_t<is_tuple_like_v<Type>>>
        void print_tuple_like(std::ostream& os, Type const& tuple)
        {
            std::apply([&os](auto const&... args) {
                size_t n {};
                ((os << (n++ != 0 ? (os.iword(pretty_index) ? ", " : " ") : "") << args), ...);
            },
                       tuple);
        }
        template <std::size_t... I, typename... Ts>
        void debug_impl(std::vector<std::string_view> const& names, std::index_sequence<I...>, Ts const&... args)
        {
            using str::strip;
            (putln(std::cerr, "\t", strip(names[I]), ": ", args), ...);
        }
    }
    auto operator<<(std::ostream& os, std::string_view strv) -> std::ostream&
    {
        if (os.iword(pretty_index)) {
            os << "\"";
        }
        std::operator<<(os, strv);
        if (os.iword(pretty_index)) {
            os << "\"";
        }
        return os;
    }
    auto operator<<(std::ostream& os, std::string const& str) -> std::ostream&
    {
        if (os.iword(pretty_index)) {
            os << "\"";
        }
        std::operator<<(os, str);
        if (os.iword(pretty_index)) {
            os << "\"";
        }
        return os;
    }
    template <typename... Params>
    auto operator<<(std::ostream& os, std::vector<Params...> const& vector) -> std::ostream&
    {
        if (os.iword(pretty_index)) {
            os << "[";
        }
        auto first = std::begin(vector);
        auto last  = std::end(vector);
        while (first != last) {
            os << *first++;
            if (first != last) {
                os << (os.iword(pretty_index) ? ", " : " ");
            } else {
                break;
            }
        }
        if (os.iword(pretty_index)) {
            os << "]";
        }
        return os;
    }
    template <typename... Params>
    auto operator<<(std::ostream& os, std::set<Params...> const& set) -> std::ostream&
    {
        if (os.iword(pretty_index)) {
            os << "{";
        }
        auto first = std::begin(set);
        auto last  = std::end(set);
        while (first != last) {
            os << *first++;
            if (first != last) {
                os << (os.iword(pretty_index) ? ", " : " ");
            } else {
                break;
            }
        }
        if (os.iword(pretty_index)) {
            os << "}";
        }
        return os;
    }
    template <typename... Params>
    auto operator<<(std::ostream& os, std::unordered_set<Params...> const& uset) -> std::ostream&
    {
        if (os.iword(pretty_index)) {
            os << "{";
        }
        auto first = std::begin(uset);
        auto last  = std::end(uset);
        while (first != last) {
            os << *first++;
            if (first != last) {
                os << (os.iword(pretty_index) ? ", " : " ");
            } else {
                break;
            }
        }
        if (os.iword(pretty_index)) {
            os << "}";
        }
        return os;
    }
    template <typename... Params>
    auto operator<<(std::ostream& os, std::map<Params...> const& map) -> std::ostream&
    {
        if (os.iword(pretty_index)) {
            os << "{";
        }
        auto first = std::begin(map);
        auto last  = std::end(map);
        while (first != last) {
            os << first->first << (os.iword(pretty_index) ? ": " : " ") << first->second;
            first++;
            if (first != last) {
                os << (os.iword(pretty_index) ? ", " : " ");
            } else {
                break;
            }
        }
        if (os.iword(pretty_index)) {
            os << "}";
        }
        return os;
    }
    template <typename... Params>
    auto operator<<(std::ostream& os, std::unordered_map<Params...> const& umap) -> std::ostream&
    {
        if (os.iword(pretty_index)) {
            os << "{";
        }
        auto first = std::begin(umap);
        auto last  = std::end(umap);
        while (first != last) {
            os << first->first << (os.iword(pretty_index) ? ": " : " ") << first->second;
            first++;
            if (first != last) {
                os << (os.iword(pretty_index) ? ", " : " ");
            } else {
                break;
            }
        }
        if (os.iword(pretty_index)) {
            os << "}";
        }
        return os;
    }
    template <typename T, std::size_t N>
    auto operator<<(std::ostream& os, std::array<T, N> const& array) -> std::ostream&
    {
        if (os.iword(pretty_index)) {
            os << "[";
        }
        detail::print_tuple_like(os, array);
        if (os.iword(pretty_index)) {
            os << "]";
        }
        return os;
    }
    template <typename... Ts>
    auto operator<<(std::ostream& os, std::tuple<Ts...> const& tuple) -> std::ostream&
    {
        if (os.iword(pretty_index)) {
            os << "(";
        }
        detail::print_tuple_like(os, tuple);
        if (os.iword(pretty_index)) {
            os << ")";
        }
        return os;
    }
    template <typename T, typename U>
    auto operator<<(std::ostream& os, std::pair<T, U> const& pair) -> std::ostream&
    {
        if (os.iword(pretty_index)) {
            os << "(";
        }
        detail::print_tuple_like(os, pair);
        if (os.iword(pretty_index)) {
            os << ")";
        }
        return os;
    }
    auto operator<<(std::ostream& os, ds::mint const& m) -> std::ostream& { return os << m.value(); }
    template <typename... Ts>
    void scan(std::istream& is, Ts&... args)
    {
        using utils::is_scannable;
        static_assert(sizeof...(Ts) > 0, "need at least one element to scan");
        static_assert(std::conjunction_v<is_scannable<Ts>...>, "not all types are scannable"); // GCC 11.1 bug (hackerrank, cses) -- causes compilation error something related to ADL
        auto holder = std::make_tuple(std::ref(args)...);
        is >> holder;
    }
    void scanln(std::istream& is, std::string& line)
    {
        std::getline(is >> std::ws, line);
    }
    template <char... Seps, typename T, typename>
    void put(std::ostream& os, T first, T last)
    {
        if (first == last) {
            return;
        }
        os << *first++;
        while (first != last) {
            if constexpr (sizeof...(Seps) == 0) {
                os << ' ';
            } else {
                (os << ... << Seps);
            }
            os << *first++;
        }
    }
    template <char... Seps, typename... Ts>
    void put(std::ostream& os, Ts const&... args)
    {
        bool first { true };
        if constexpr (sizeof...(Seps) == 0) {
            ((first ? (first = false, os << args) : (os << ' ' << args)), ...);
        } else if constexpr (sizeof...(Seps) == 1 && std::get<0>(std::make_tuple(Seps...)) == 0) {
            (os << ... << args);
        } else {
            ((first ? (first = false, os << args) : (os << ... << Seps) << args), ...);
        }
    }
    template <typename... Ts>
    void debug(char const* name_str, Ts const&... args)
    {
        putln(std::cerr, "{");
        auto names = str::split(name_str, ",");
        std::cout << pretty;
        detail::debug_impl(names, std::make_index_sequence<sizeof...(args)> {}, args...);
        std::cout << pretty;
        putln(std::cerr, "}");
    }
}
}
using namespace speed;
using ds::dsu;
using ds::mint;
using io::put;
using io::putln;
using io::scan;
using io::scanln;
using math::binary_expo;
using math::binomial_coeff;
using math::ceil;
using math::divisors;
using math::factorial;
using math::floor;
using math::is_prime;
using math::nmax;
using math::nmin;
using str::split;
using str::strip;
// for ADL lookup -- workaround for the compiler bug
using io::operator<<;
using io::operator>>;
#define DEBUG(...)                            \
    do {                                      \
        io::debug(#__VA_ARGS__, __VA_ARGS__); \
    } while (false)
using namespace std;
using i32 = int32_t;
using i64 = int64_t;
using u32 = uint32_t;
using u64 = uint64_t;
int main()
{
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);

    int T {};
    for (scan(T); T--;) {
    }
}