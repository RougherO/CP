#pragma once
// Counts live objects so containers can be checked for double/missing destruction.
struct tracked {
    static inline int alive = 0;
    int v                   = 0;
    tracked(int x = 0) : v(x) { ++alive; }
    tracked(tracked const& o) : v(o.v) { ++alive; }
    tracked(tracked&& o) noexcept : v(o.v) { ++alive; }
    auto operator=(tracked const&) -> tracked&     = default;
    auto operator=(tracked&&) noexcept -> tracked& = default;
    ~tracked() { --alive; }
    friend auto operator==(tracked const& a, tracked const& b) { return a.v == b.v; }
    friend auto operator!=(tracked const& a, tracked const& b) { return a.v != b.v; }
    friend auto operator<(tracked const& a, tracked const& b) { return a.v < b.v; }
};
