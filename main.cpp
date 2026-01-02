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
using i8    = int8_t;
using i16   = int16_t;
using i32   = int32_t;
using i64   = int64_t;
using u8    = uint8_t;
using u16   = int16_t;
using u32   = uint32_t;
using u64   = uint64_t;
using isize = std::ptrdiff_t;
using usize = std::size_t;
auto operator""_i8(unsigned long long x) -> i8 { return x; }
auto operator""_i16(unsigned long long x) -> i16 { return x; }
auto operator""_i32(unsigned long long x) -> i32 { return x; }
auto operator""_i64(unsigned long long x) -> i64 { return x; }
auto operator""_u8(unsigned long long x) -> u8 { return x; }
auto operator""_u16(unsigned long long x) -> u16 { return x; }
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
    class reader;
    class writer;
    template <typename T, typename = void>
    struct serializer;
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
    namespace detail {
        template <typename T, typename... Args>
        auto is_scannable_helper(int) -> decltype(io::serializer<T>::read(
                                                      std::declval<io::reader&>(),
                                                      std::declval<T&>(),
                                                      std::declval<std::forward<Args>>()...),
                                                  std::true_type {});
        template <typename>
        auto is_scannable_helper(...) -> std::false_type;
        template <typename T, typename... Args>
        auto is_printable_helper(int) -> decltype(io::serializer<T>::write(
                                                      std::declval<io::writer&>(),
                                                      std::declval<T const&>(),
                                                      std::declval<std::forward<Args>>()...),
                                                  std::true_type {});
        template <typename>
        auto is_printable_helper(...) -> std::false_type;
    }
    template <typename T>
    using is_scannable = decltype(detail::is_scannable_helper<T>(0));
    template <typename T>
    bool constexpr is_scannable_v = is_scannable<T>::value;
    template <typename T>
    using is_printable = decltype(detail::is_printable_helper<T>(0));
    template <typename T>
    bool constexpr is_printable_v = is_printable<T>::value;
}
namespace io {
    template <usize Capacity>
    struct input_buffer {
        friend serializer<void>;
        friend reader;
        input_buffer(std::istream& is)
            : m_is { is }
        {
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
        friend writer;
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
        // Copy `capacity` bytes to `dest` based on a delimeter `f`
        // Returns true if more bytes are left to be scanned for the next token
        // Should be called repeatedly in a while loop
        template <usize Capacity, typename DelimeterCallback>
        static auto next_token(input_buffer<Capacity>&, char*&, usize, DelimeterCallback&&) -> bool;
        // Same as above except gets called with default delimeter of whitespace
        template <usize Capacity>
        static auto next_token(input_buffer<Capacity>&, char*&, usize) -> bool;
    };
    template <>
    struct serializer<char> {
        static void read(reader&, char&);
        static void write(writer&, char const&);
    };
    template <typename T>
    struct serializer<T, std::enable_if_t<std::is_integral_v<T>>> {
        inline static char digits[128] {};
        static void read(reader&, T&);
        static void write(writer&, T const&);
    };
    template <typename T>
    struct serializer<T, std::enable_if_t<std::is_floating_point_v<T>>> {
        inline static char digits[128] {};
        static void read(reader&, T&);
        static void write(writer&, T const&);
    };
    template <usize N>
    struct serializer<char[N]> {
        static void read(reader&, char (&)[N]);
        static void write(writer&, char const (&)[N]);
    };
    template <>
    struct serializer<std::string> {
        inline static char buf[128] {};
        static void read(reader&, std::string&);
        static void write(writer&, std::string const&);
    };
    template <>
    struct serializer<std::string_view> {
        static void write(writer&, std::string_view const&);
    };
    template <typename T>
    struct serializer<std::vector<T>> {
        static auto read(reader&, std::vector<T>&) -> std::enable_if_t<utils::is_scannable_v<T>>;
        static auto write(writer&, std::vector<T> const&) -> std::enable_if_t<utils::is_printable_v<T>>;
    };
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
    class reader {
    public:
        reader(std::istream& is)
            : m_buffer { is }
        {
        }
        auto eof() const noexcept -> bool { return m_buffer.m_count == 0; }
        template <typename... Ts>
        void read(Ts&... args) { (m_read_value(args), ...); }
        // special handling for fixed size buffered input
        // returns true when more characters are still part of the current line
        // false when eof or newline is encountered
        // Reads upto N - 1 characters and puts a null at the end
        template <usize N>
        auto readln(char (&line)[N]) -> bool
        {
            char* ptr           = line;
            bool has_more_chars = serializer<void>::next_token(m_buffer, ptr, N - 1, [](char c) { return c == '\r' || c == '\n'; });
            *ptr                = 0;
            return has_more_chars;
        }
        void readln(std::string& line)
        {
            line.clear();
            line.resize(16);

            char* ptr = line.data();
            while (serializer<void>::next_token(m_buffer, ptr, line.size() / 2,
                                                [](char c) { return c == '\r' || c == '\n'; })) {
                line.resize(line.size() * 2);
                ptr = line.data() + line.size() / 2;
            }
        }
        void set_float_parse_fmt(std::chars_format fmt) noexcept { m_float_parse_fmt = fmt; }
        void set_integer_parse_base(int base) noexcept { m_int_parse_base = base; }
        auto get_float_parse_fmt() const noexcept -> std::chars_format { return m_float_parse_fmt; }
        auto get_integer_parse_base() const noexcept -> int { return m_int_parse_base; }
        auto get_buffer() noexcept -> auto& { return m_buffer; }

    private:
        template <typename T>
        auto m_read_value(T& value) -> std::enable_if_t<utils::is_scannable_v<T>> { serializer<T>::read(*this, value); }

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
        void set_float_precision(int precision) noexcept { m_float_precision = precision; }
        void set_float_print_fmt(std::chars_format fmt) noexcept { m_float_print_fmt = fmt; }
        void set_integer_print_base(int base) noexcept { m_int_print_base = base; }
        auto get_float_precision() const noexcept -> int { return m_float_precision; }
        auto get_float_print_fmt() const noexcept -> std::chars_format { return m_float_print_fmt; }
        auto get_integer_print_base() const noexcept -> int { return m_int_print_base; }
        auto get_buffer() noexcept -> auto& { return m_buffer; }

    private:
        template <typename T>
        auto m_write_value(T const& value) -> std::enable_if_t<utils::is_printable_v<T>> { serializer<T>::write(*this, value); }

        output_buffer<1 << 16> m_buffer;
        int m_int_print_base  = 10;
        int m_float_precision = 2;
        std::chars_format m_float_print_fmt { std::chars_format::fixed };
    };
    template <usize Capacity, typename DelimeterCallback>
    auto serializer<void>::next_token(input_buffer<Capacity>& buffer, char*& dest, usize capacity, DelimeterCallback&& f) -> bool
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
        if (count == capacity && !f(buffer.m_buffer[buffer.m_read])) {
            return true;
        }
        return false;
    }
    template <usize Capacity>
    auto serializer<void>::next_token(input_buffer<Capacity>& buffer, char*& dest, usize capacity) -> bool
    {
        return next_token(buffer, dest, capacity, [](char c) { return std::isspace(c); });
    }
    void serializer<char>::read(reader& r, char& c)
    {
        char* ptr = &c;
        serializer<void>::next_token(r.get_buffer(), ptr, 1);
    }
    void serializer<char>::write(writer& w, char const& c) { w.get_buffer().write(&c, 1); }
    template <typename T>
    void serializer<T, std::enable_if_t<std::is_integral_v<T>>>::read(reader& r, T& value)
    {
        auto& buffer = r.get_buffer();
        char* ptr    = digits;
        while (serializer<void>::next_token(buffer, ptr, sizeof(digits) - (ptr - digits))) { }

        auto [_, ec] = std::from_chars(digits, ptr, value, r.get_integer_parse_base());
        if (ec != std::errc {}) {
            value = 0;
        }
    }
    template <typename T>
    void serializer<T, std::enable_if_t<std::is_integral_v<T>>>::write(writer& w, T const& value)
    {
        auto [ptr, _] = std::to_chars(digits, digits + sizeof(digits), value, w.get_integer_print_base());
        w.get_buffer().write(digits, ptr - digits);
    }
    template <typename T>
    void serializer<T, std::enable_if_t<std::is_floating_point_v<T>>>::read(reader& r, T& value)
    {
        auto& buffer = r.get_buffer();
        char* ptr    = digits;
        while (serializer<void>::next_token(buffer, ptr, sizeof(digits) - (ptr - digits))) { }

        auto [_, ec] = std::from_chars(digits, ptr, value, r.get_float_parse_fmt());
        if (ec != std::errc {}) {
            value = 0;
        }
    }
    template <typename T>
    void serializer<T, std::enable_if_t<std::is_floating_point_v<T>>>::write(writer& w, T const& value)
    {
        auto [ptr, _] = std::to_chars(digits, digits + sizeof(digits), value, w.get_float_print_fmt(), w.get_float_precision());
        w.get_buffer().write(digits, ptr - digits);
    }
    template <usize N>
    void serializer<char[N]>::read(reader& r, char (&value)[N])
    {
        static_assert(N > 1, "not enough buffer space. Expect size for at least 2 chars including null");
        char* ptr = value;
        serializer<void>::next_token(r.get_buffer(), ptr, N - 1, [](char) { return true; });
        *ptr = 0;
    }
    template <usize N>
    void serializer<char[N]>::write(writer& w, char const (&value)[N]) { w.get_buffer().write(value, strnlen(value, N)); }
    void serializer<std::string>::read(reader& r, std::string& value)
    {
        auto& buffer = r.get_buffer();
        char* ptr = buf;
        while (serializer<void>::next_token(buffer, ptr, sizeof(buf))) {
            value.append(buf, ptr);
            ptr = buf;
        }
        value.append(buf, ptr);
    }
    void serializer<std::string>::write(writer& w, std::string const& value) { w.get_buffer().write(value.data(), value.size()); }
    void serializer<std::string_view>::write(writer& w, std::string_view const& value) { w.get_buffer().write(value.data(), value.size()); }
    template <typename T>
    auto serializer<std::vector<T>>::read(reader& r, std::vector<T>& value) -> std::enable_if_t<utils::is_scannable_v<T>>
    {
        for (T& e : value) {
            r.read(e);
        }
    }
    template <typename T>
    auto serializer<std::vector<T>>::write(writer& w, std::vector<T> const& value) -> std::enable_if_t<utils::is_printable_v<T>>
    {
        bool first = true;
        for (T const& e : value) {
            if (first) {
                w.write(e);
                first = false;
            } else {
                w.write(' ');
                w.write(e);
            }
        }
    }
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
        vector<i32> p(n);
        read(p);

        writeln(p);
    }
}