#include <bits/stdc++.h>
#ifdef __cpp_lib_ranges
namespace r = std::ranges;
namespace v = std::views;
#endif
namespace speed {
namespace ds {
    template <long long = 1000000007>
    struct mint;
    struct dsu;
}
namespace algo {
    template <typename Container = std::vector<std::string>>
    auto split(std::string const&, std::string_view = "") -> Container;
}
namespace math {
    template <typename T>
    constexpr auto binary_expo(T, unsigned long long) noexcept -> T;
    template <typename T>
    constexpr auto binomial_coeff(T n, T r) -> T;
    template <typename T>
    constexpr auto factorial(T n) -> T;
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
}
namespace io {
    static int const pretty_index = std::ios_base::xalloc(); // For pretty printing
    using utils::is_tuple_like_v;
    template <int Mod>
    auto operator>>(std::istream&, ds::mint<Mod>&) -> std::istream&;
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
    template <long long Mod>
    auto operator<<(std::ostream&, ds::mint<Mod> const&) -> std::ostream&;
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
    template <long long Mod>
    struct mint {
        static constexpr long long mod = Mod;
        constexpr mint()               = default;
        constexpr mint(long long value)
            : x { value % mod }
        {
        }
        constexpr auto operator+(mint const& o) const noexcept -> mint { return (x + o.x) % mod; }
        constexpr auto operator-(mint const& o) const noexcept -> mint { return (x - o.x) % mod + (x < o.x ? mod : 0); }
        constexpr auto operator*(mint const& o) const noexcept -> mint { return (x * o.x) % mod; }
        constexpr auto operator/(mint const& o) const noexcept -> mint { return mint { x } * math::binary_expo(o, mod - 2); }
        constexpr auto operator+=(mint const& o) noexcept -> mint&
        {
            *this = *this + o;
            return *this;
        }
        constexpr auto operator-=(mint const& o) noexcept -> mint&
        {
            *this = *this - o;
            return *this;
        }
        constexpr auto operator*=(mint const& o) noexcept -> mint&
        {
            *this = *this * o;
            return *this;
        }
        constexpr auto operator/=(mint const& o) noexcept -> mint&
        {
            *this = *this / o;
            return *this;
        }
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
        constexpr explicit operator int() const noexcept { return x; }
        constexpr void X(long long value) noexcept { x = value % Mod; }
        constexpr auto X() const noexcept -> long long { return x; }

    private:
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
namespace algo {
    template <typename Container>
    auto split(std::string const& line, std::string_view delim) -> Container
    {
        Container cont;
        if (delim.size() == 0) {
            std::stringstream ss { line };
            std::move(std::istream_iterator<std::string> { ss }, std::istream_iterator<std::string> {}, std::back_inserter(cont));
            return cont;
        }
        size_t s = 0;
        size_t e = 0;
        while ((e = line.find(delim, s)) != std::string::npos) {
            cont.insert(std::end(cont), line.substr(s, e - s));
            s = e + delim.size();
        }
        cont.insert(std::end(cont), line.substr(s));
        return cont;
    }
}
namespace math {
    template <typename T>
    constexpr auto binary_expo(T base, unsigned long long pow) noexcept -> T
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
    template <typename T>
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
    template <typename T>
    constexpr auto factorial(T n) -> T
    {
        T prod { 1 };
        for (T i = 2; i <= n; i++) {
            prod *= i;
        }
        return prod;
    }
}
namespace io {
    template <long long Mod>
    auto operator>>(std::istream& is, ds::mint<Mod>& m) -> std::istream&
    {
        long long x {};
        is >> x;
        m.X(x);
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
        void debug_impl(std::vector<std::string> const& names, std::index_sequence<I...>, Ts const&... args)
        {
            (putln("\t", names[I], ": ", args), ...);
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
    template <long long Mod>
    auto operator<<(std::ostream& os, ds::mint<Mod> const& m) -> std::ostream& { return os << m.X(); }
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
        } else {
            ((first ? (first = false, os << args) : (os << ... << Seps) << args), ...);
        }
    }
    template <typename... Ts>
    void debug(char const* name_str, Ts const&... args)
    {
        putln("{");
        auto names = algo::split(name_str, ", ");
        std::cout << pretty;
        detail::debug_impl(names, std::make_index_sequence<sizeof...(args)> {}, args...);
        std::cout << pretty;
        putln("}");
    }
}
}
using namespace speed;
using algo::split;
using ds::dsu;
using ds::mint;
using io::put;
using io::putln;
using io::scan;
using io::scanln;
using math::binary_expo;
using math::binomial_coeff;
using math::factorial;
// for ADL lookup -- workaround for the compiler bug
using io::operator<<;
using io::operator>>;
#define DEBUG(...)                            \
    do {                                      \
        io::debug(#__VA_ARGS__, __VA_ARGS__); \
    } while (false)
using namespace std;
using ll = long long;
int main()
{
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);

    int T {};
    for (scan(T); T--;) {
    }
}
