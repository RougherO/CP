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
using i32   = int32_t;
using i64   = int64_t;
using u32   = uint32_t;
using u64   = uint64_t;
using isize = std::ptrdiff_t;
using usize = std::size_t;
auto operator""_i32(unsigned long long x) -> i32 { return x; }
auto operator""_i64(unsigned long long x) -> i64 { return x; }
auto operator""_u32(unsigned long long x) -> u32 { return x; }
auto operator""_u64(unsigned long long x) -> u64 { return x; }
auto operator""_iz(unsigned long long x) -> isize { return x; }
auto operator""_uz(unsigned long long x) -> usize { return x; }
namespace ds {
    template <i64 Mod = 1'000'000'007>
    struct mint;
    struct dsu;
}
namespace str {
    auto split(std::string_view, std::string_view = "") -> std::vector<std::string_view>;
    auto strip(std::string_view) -> std::string_view;
}
namespace io {
    template <typename T, typename = void>
    struct serializer;
    template <usize Capacity>
    struct input_buffer;
    template <usize Capacity>
    struct output_buffer;
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
    struct is_iterator_like : std::false_type { };
    template <typename T>
    struct is_iterator_like<T, std::void_t<decltype(*std::declval<T>()),
                                           decltype(++std::declval<T>()),
                                           decltype(std::declval<T>()++)>> : std::true_type { };
    template <typename T>
    constexpr bool is_iterator_like_v = is_iterator_like<T>::value;
    template <typename T>
    struct is_integer : std::is_integral<T> { };
    template <i64 Mod>
    struct is_integer<ds::mint<Mod>> : std::true_type { };
    template <typename T>
    constexpr bool is_integer_v = is_integer<T>::value;
    template <typename T, typename = void>
    struct is_scannable : std::false_type { };
    template <typename T>
    struct is_scannable<T, std::void_t<decltype(io::serializer<T>::read(
                               std::declval<io::input_buffer<0>&>(),
                               std::declval<T&>()))>> : std::true_type { };
    template <typename T>
    bool constexpr is_scannable_v = is_scannable<T>::value;
    template <typename T, typename = void>
    struct is_printable : std::false_type { };
    template <typename T>
    struct is_printable<T, std::void_t<decltype(io::serializer<T>::write(
                               std::declval<io::output_buffer<0>&>(),
                               std::declval<T const&>()))>> : std::true_type { };
    template <typename T>
    bool constexpr is_printable_v = is_printable<T>::value;
}
namespace algo {
    // Does a conditional binary search with a conditional
    // function over an integer range or iterator range
    template <typename T, typename Cond>
    auto cond_binary_search(T, T, Cond&&) -> T;
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
    constexpr auto sqr(T) -> T;
    template <typename T, typename = std::enable_if_t<is_integer_v<T>>>
    constexpr auto isqrt(T) -> T;
    template <typename T, typename = std::enable_if_t<is_integer_v<T>>>
    constexpr auto lclamp(T, T) -> T;
    template <typename T, typename = std::enable_if_t<is_integer_v<T>>>
    constexpr auto uclamp(T, T) -> T;
    template <typename T, typename = std::enable_if_t<is_integer_v<T>>>
    auto divisors(T) -> std::vector<T>;
    template <typename T>
    constexpr auto nmax = std::numeric_limits<T>::max();
    template <typename T>
    constexpr auto nmin = std::is_floating_point_v<T> ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::min();
}
namespace ds {
    template <i64 Mod>
    struct mint {
        constexpr mint() = default;
        constexpr mint(i64 value)
            : x { value % Mod }
        {
        }
        friend constexpr auto operator+(mint const& l, mint const& r) noexcept -> mint { return (l.x + r.x) % Mod; }
        friend constexpr auto operator-(mint const& l, mint const& r) noexcept -> mint { return (l.x - r.x) % Mod + (l.x < r.x ? Mod : 0); }
        friend constexpr auto operator*(mint const& l, mint const& r) noexcept -> mint { return (l.x * r.x) % Mod; }
        friend constexpr auto operator/(mint const& l, mint const& r) noexcept -> mint { return l.x * math::binary_expo(r, Mod - 2); }
        friend constexpr auto operator%(mint const& l, mint const& r) noexcept -> mint { return l.x % r.x; }
        friend constexpr auto operator<<(mint const& l, mint const& r) noexcept -> mint { return (l.x << r.x) % Mod; }
        friend constexpr auto operator>>(mint const& l, mint const& r) noexcept -> mint { return (l.x >> r.x) % Mod; }
        friend constexpr auto operator|(mint const& l, mint const& r) noexcept -> mint { return l.x | r.x; }
        friend constexpr auto operator&(mint const& l, mint const& r) noexcept -> mint { return l.x & r.x; }
        constexpr auto operator~() const noexcept -> mint { return ~x % Mod; }
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
        constexpr auto value() const noexcept -> i64 { return x; }

    private:
        i64 x {};
    };
    struct dsu {
        dsu(usize n)
            : m_rank(n)
            , m_parent(n)
        {
            std::iota(m_parent.begin(), m_parent.end(), 0);
        }
        auto parent(usize node) noexcept -> usize
        {
            if (m_parent[node] == node) {
                return node;
            }
            return m_parent[node] = parent(m_parent[node]);
        }
        auto unite(usize node1, usize node2) noexcept -> bool
        {
            usize p1 = parent(node1);
            usize p2 = parent(node2);
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
        auto count() const noexcept -> usize
        {
            usize n = m_parent.size();
            usize cnt {};
            for (usize i = 0; i != n; ++i) {
                if (i == m_parent[i]) {
                    cnt++;
                }
            }
            return cnt;
        }

    private:
        std::vector<usize> m_rank;
        std::vector<usize> m_parent;
    };
}
namespace str {
    auto split(std::string_view line, std::string_view delim) -> std::vector<std::string_view>
    {
        std::vector<std::string_view> cont;
        usize s = 0;
        usize e = 0;
        if (delim.size() == 0) {
            usize size = line.size();
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
        usize l = word.find_first_not_of(' ');
        usize r = word.find_last_not_of(' ');
        return word.substr(l, r - l + 1);
    }
}
namespace algo {
    namespace detail {
        template <typename T, typename Cond>
        auto cond_binary_search_integer_range(T begin, T end, Cond&& cond) -> T
        {
            auto step = (end - begin) / 2;
            while (begin != end) {
                T mid = begin + step;
                if (cond(mid)) {
                    begin = mid + 1;
                } else {
                    end = mid;
                }
                step /= 2;
            }
            // T T T T T F F F F F F F
            //           ^------- position of `begin`
            // `begin` at position where cond always
            // returns false
            return begin;
        }
        template <typename T, typename Cond>
        auto cond_binary_search_iterator_range(T begin, T end, Cond&& cond) -> T
        {
            auto step = std::distance(begin, end) / 2;
            while (begin != end) {
                T mid = std::next(begin, step);
                if (cond(*mid)) {
                    begin = std::next(mid);
                } else {
                    end = mid;
                }
                step /= 2;
            }
            return begin;
        }
    }
    template <typename T, typename Cond>
    auto cond_binary_search(T begin, T end, Cond&& cond) -> T
    {
        static_assert(std::is_invocable_r_v<T, Cond, T>, "Wrong function signature type");
        if constexpr (utils::is_integer_v<T>) {
            return detail::cond_binary_search_integer_range(begin, end, std::forward<Cond>(cond));
        } else if constexpr (utils::is_iterator_like_v<T>) {
            return detail::cond_binary_search_iterator_range(begin, end, std::forward<Cond>(cond));
        } else {
            static_assert(false, "Conditional binary search supported only with integer or iterator ranges");
        }
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
    constexpr auto floor(T x, T y) -> T { return x / y; }
    template <typename T, typename>
    constexpr auto ceil(T x, T y) -> T { return (x + y - 1) / y; }
    template <typename T, typename>
    constexpr auto sqr(T x) -> T { return x * x; }
    template <typename T, typename>
    constexpr auto isqrt(T x) -> T
    {
        if (x < 2) {
            return x;
        }
        T mi = T { 1 };
        T ma = nmax<T>();
        while (mi < ma) {
            T mid = std::midpoint(mi, ma);
            if (mid <= x / mid) {
                mi = mid + 1;
            } else {
                ma = mid;
            }
        }
        // mi > x (can be proved)
        return mi--;
    }
    template <typename T, typename>
    constexpr auto lclamp(T x, T l) -> T { return std::clamp(x, l, nmax<T>()); }
    template <typename T, typename>
    constexpr auto uclamp(T x, T u) -> T { return std::clamp(x, nmin<T>(), u); }
    template <typename T, typename>
    auto divisors(T x) -> std::vector<T>
    {
        T n { 1 };
        std::vector<T> divs_first;
        std::vector<T> divs_second;
        while (n <= x / n) {
            if (x % n == 0) {
                divs_first.push_back(n);
                divs_second.push_back(x / n);
            }
            n++;
        }
        divs_first.insert(divs_first.end(), divs_second.rbegin(), divs_second.rend());
        return divs_first;
    }
}
namespace io {
    using utils::is_printable_v;
    using utils::is_scannable_v;
    template <usize Capacity>
    struct input_buffer {
        friend serializer<void>;
        friend serializer<char>; // only char gets special treatment
        input_buffer(std::istream& is)
            : m_is { is }
        {
        }
        void read(char* dest, usize size)
        {
            usize copied = 0;
            while (copied < size) {
                auto available = m_count - m_read;
                if (available == 0) {
                    m_refill();
                    available = m_count - m_count;
                    if (available == 0) {
                        return;
                    }
                }
                auto copy_count = std::min(size - copied, available);
                std::copy_n(m_buffer + m_read, copy_count, dest + copied);

                m_read += copy_count;
                copied += copy_count;
            }
        }

    private:
        void m_refill()
        {
            m_read = 0;
            m_is.read(m_buffer, Capacity);
            m_count = m_is.gcount();
        }
        std::istream& m_is;
        char m_buffer[Capacity] {};
        usize m_read {};
        usize m_count {};
    };
    template <usize Capacity>
    struct output_buffer {
        output_buffer(std::ostream& os)
            : m_os { os }
        {
        }
        void write(char const* src, usize size)
        {
            auto available = Capacity - m_size;
            if (size > available) {
                m_flush();
                m_os.write(src, size);
                return;
            }
            std::copy_n(src, size, m_buffer + m_size);
            m_size += size;
        }
        ~output_buffer()
        {
            m_flush();
        }

    private:
        void m_flush()
        {
            m_os.write(m_buffer, m_size);
            m_size = 0;
        }
        std::ostream& m_os;
        char m_buffer[Capacity] {};
        usize m_size {};
    };
    template <>
    struct serializer<void> {
        template <usize Capacity>
        static auto next_token(input_buffer<Capacity>& buffer, char*& dest, usize capacity) -> bool
        {
            return next_token(buffer, dest, capacity, [](char c) { return std::isspace(c); });
        }
        template <usize Capacity, typename DelimeterCallback>
        static auto next_token(input_buffer<Capacity>& buffer, char*& dest, usize capacity, DelimeterCallback&& f) -> bool
        {
            char* first;
            while (true) {
                first = std::find_if_not(buffer.m_buffer + buffer.m_read, buffer.m_buffer + buffer.m_count, f);
                if (first != buffer.m_buffer + buffer.m_count) {
                    buffer.m_read += std::distance(buffer.m_buffer + buffer.m_read, first);
                    break;
                }
                buffer.m_refill();
                if (buffer.m_count == 0) {
                    return false;
                }
            }
            char* last     = std::find_if(first, buffer.m_buffer + buffer.m_count, f);
            auto count     = std::min<usize>(last - first, capacity);
            dest           = std::copy_n(first, count, dest);
            buffer.m_read += count;
            if (buffer.m_read == buffer.m_count) {
                buffer.m_refill();
                if (buffer.m_count != 0 && !f(buffer.m_buffer[0])) {
                    return true;
                }
            }
            return false;
        }
    };
    template <>
    struct serializer<char> {
        template <usize Capacity>
        static void read(input_buffer<Capacity>& buffer, char& c)
        {
            if (buffer.m_read == buffer.m_count) {
                buffer.m_refill();
                if (buffer.m_count == 0) {
                    c = 0;
                    return;
                }
            }
            c = buffer.m_buffer[buffer.m_read++];
        }
        template <usize Capacity>
        static void write(output_buffer<Capacity>& buffer, char const& c)
        {
            buffer.write(&c, sizeof(char));
        }
    };
    template <typename T>
    struct serializer<T, std::enable_if_t<std::is_integral_v<T>>> {
        inline static char digits[128] {};
        template <usize Capacity>
        static void read(input_buffer<Capacity>& buffer, T& value)
        {
            char* ptr = digits;
            while (serializer<void>::next_token(buffer, ptr, sizeof(digits) - (ptr - digits))) { }

            auto [_, ec] = std::from_chars(digits, ptr, value, 10);
            if (ec != std::errc {}) {
                value = 0;
            }
        }
        template <usize Capacity>
        static void write(output_buffer<Capacity>& buffer, T const& value)
        {
            auto [ptr, _] = std::to_chars(digits, digits + sizeof(digits), value, 10);
            buffer.write(digits, ptr - digits);
        }
        template <usize Capacity>
        static void read(input_buffer<Capacity>& buffer, T& value, int parse_base)
        {
            char* ptr = digits;
            while (serializer<void>::next_token(buffer, ptr, sizeof(digits) - (ptr - digits))) { }

            auto [_, ec] = std::from_chars(digits, ptr, value, parse_base);
            if (ec != std::errc {}) {
                value = 0;
            }
        }
        template <usize Capacity>
        static void write(output_buffer<Capacity>& buffer, T const& value, int print_base)
        {
            auto [ptr, _] = std::to_chars(digits, digits + sizeof(digits), value, print_base);
            buffer.write(digits, ptr - digits);
        }
    };
template <typename T>
struct serializer<T, std::enable_if_t<std::is_floating_point_v<T>>> {
    inline static char digits[128] {};
    template <usize Capacity>
    static void read(input_buffer<Capacity>& buffer, T& value)
    {
        char* ptr = digits;
        while (serializer<void>::next_token(buffer, ptr, sizeof(digits) - (ptr - digits))) { }

        auto [_, ec] = std::from_chars(digits, ptr, value, std::chars_format::fixed);
        if (ec != std::errc {}) {
            value = 0;
        }
    }
    template <usize Capacity>
    static void write(output_buffer<Capacity>& buffer, T const& value)
    {
        auto [ptr, _] = std::to_chars(digits, digits + sizeof(digits), value, std::chars_format::fixed, 2);
        buffer.write(digits, ptr - digits);
    }
    template <usize Capacity>
    static void read(input_buffer<Capacity>& buffer, T& value, std::chars_format parse_fmt)
    {
        char* ptr = digits;
        while (serializer<void>::next_token(buffer, ptr, sizeof(digits) - (ptr - digits))) { }

        auto [_, ec] = std::from_chars(digits, ptr, value, parse_fmt);
        if (ec != std::errc {}) {
            value = 0;
        }
    }
    template <usize Capacity>
    static void write(output_buffer<Capacity>& buffer, T const& value, int precision, std::chars_format print_fmt)
    {
        auto [ptr, _] = std::to_chars(digits, digits + sizeof(digits), value, print_fmt, precision);
        buffer.write(digits, ptr - digits);
    }
};
template <>
struct serializer<std::string> {
    template <usize Capacity>
    static void read(input_buffer<Capacity>& buffer, std::string& value)
    {
        char buf[1024];
        char* ptr = buf;
        while (serializer<void>::next_token(buffer, ptr, sizeof(buf))) {
            value.append(buf, ptr);
            ptr = buf;
        }
        value.append(buf, ptr);
    }
    template <usize Capacity>
    static void readln(input_buffer<Capacity>& buffer, std::string& value)
    {
        char buf[1024];
        char* ptr = buf;
        while (serializer<void>::next_token(buffer, ptr, sizeof(buf), [](char c) { return c == '\r' || c == '\n'; })) {
            value.append(buf, ptr);
            ptr = buf;
        }
        value.append(buf, ptr);
    }
    template <usize Capacity>
    static void write(output_buffer<Capacity>& buffer, std::string const& value)
    {
        buffer.write(value.data(), value.size());
    }
};
template <>
struct serializer<std::string_view> {
    template <usize Capacity>
    static void write(output_buffer<Capacity>& buffer, std::string_view const& value)
    {
        buffer.write(value.data(), value.size());
    }
};
template <typename T>
struct serializer<std::vector<T>> {
    template <usize Capacity>
    static auto read(input_buffer<Capacity>& buffer, std::vector<T>& value) -> std::enable_if_t<is_scannable_v<T>>
    {
        for (T& e : value) {
            serializer<T>::read(buffer, e);
        }
    }
    template <usize Capacity>
    static auto write(output_buffer<Capacity>& buffer, std::vector<T> const& value) -> std::enable_if_t<is_printable_v<T>>
    {
        bool first = true;
        for (T const& e : value) {
            if (first) {
                serializer<T>::write(buffer, e);
                first = false;
            } else {
                serializer<char>::write(buffer, ' ');
                serializer<T>::write(buffer, e);
            }
        }
    }
};
class reader {
public:
    reader(std::istream& is)
        : m_buffer { is }
    {
    }
    template <typename... Ts>
    void read(Ts&... args) { (m_read_value(args), ...); }
    void readln(std::string& line)
    {
        line.clear();
        serializer<std::string>::readln(m_buffer, line);
    }
    void set_float_parse_fmt(std::chars_format fmt) noexcept { m_float_parse_fmt = fmt; }
    void set_integer_parse_base(int base) noexcept { m_int_parse_base = base; }

private:
    template <typename T>
    void m_read_value(T& value)
    {
        if constexpr (!is_scannable_v<T>) {
            static_assert(false, "type is not scannable");
        } else if constexpr (std::is_floating_point_v<T>) {
            serializer<T>::read(m_buffer, value, m_float_parse_fmt);
        } else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, char>) {
            serializer<T>::read(m_buffer, value, m_int_parse_base);
        } else {
            serializer<T>::read(m_buffer, value);
        }
    }
    input_buffer<1 << 16> m_buffer;
    int m_int_parse_base = 10;
    std::chars_format m_float_parse_fmt { std::chars_format::fixed };
};
class writer {
public:
    writer(std::ostream& os)
        : m_buffer(os)
    {
    }
    template <typename... Ts>
    void write_with_separator(std::string_view separator, Ts const&... args)
    {
        bool first = true;
        ((first ? (first = false, m_write_value(args))
                : (m_write_value(separator), m_write_value(args))),
         ...);
    }
    template <typename... Ts>
    void writeln_with_separator(std::string_view separator, Ts const&... args)
    {
        write_with_separator(separator, args...);
#if _WIN32 || _WIN64
        write('\r');
        write('\n');
#else
        write('\n');
#endif
    }
    template <typename... Ts>
    void write(Ts const&... args) { write_with_separator(" ", args...); }
    template <typename... Ts>
    void writeln(Ts const&... args)
    {
        write(args...);
#if _WIN32 || _WIN64
        write('\r');
        write('\n');
#else
        write('\n');
#endif
    }
    void set_float_precision(int precision) noexcept { m_float_precision = precision; }
    void set_float_print_fmt(std::chars_format fmt) noexcept { m_float_print_fmt = fmt; }
    void set_integer_print_base(int base) noexcept { m_int_print_base = base; }

private:
    template <typename T>
    void m_write_value(T const& value)
    {
        if constexpr (!is_printable_v<T>) {
            static_assert(false, "type is not printable");
        } else if constexpr (std::is_floating_point_v<T>) {
            serializer<T>::write(m_buffer, value, m_float_precision, m_float_print_fmt);
        } else if constexpr (std::is_integral_v<T> && !std::is_same_v<T, char>) {
            serializer<T>::write(m_buffer, value, m_int_print_base);
        } else {
            serializer<T>::write(m_buffer, value);
        }
    }

    output_buffer<1 << 16> m_buffer;
    int m_int_print_base  = 10;
    int m_float_precision = 2;
    std::chars_format m_float_print_fmt { std::chars_format::fixed };
};
reader rr { std::cin };
writer ww { std::cout };
}
}
using namespace speed;
using algo::cond_binary_search;
using ds::dsu;
using ds::mint;
using io::rr;
using io::ww;
using math::binary_expo;
using math::binomial_coeff;
using math::ceil;
using math::divisors;
using math::factorial;
using math::floor;
using math::is_prime;
using math::isqrt;
using math::lclamp;
using math::nmax;
using math::nmin;
using math::sqr;
using math::uclamp;
using str::split;
using str::strip;
// for ADL lookup -- workaround for the compiler bug
using namespace std;

template <typename... Ts>
void read(Ts&... args) { rr.read(args...); }
template <typename... Ts>
void write(Ts const&... args) { ww.write(args...); }
template <typename... Ts>
void writeln(Ts const&... args) { ww.writeln(args...); }

int main()
{
    // ios_base::sync_with_stdio(false);
    // cin.tie(nullptr);

    int T;
    for (read(T); T--;) {
        i32 n;
        read(n);
        i32 t = (n * (n - 1)) / 2;
        vector<i32> v(t);
        read(v);

        r::sort(v, greater {});

        vector<i32> b(n);
        for (i32 i = 0, l = 1; l != n; i += l, l++) {
            b[l - 1] = v[i];
        }
        b.back() = b.front();

        writeln(b);
    }
}