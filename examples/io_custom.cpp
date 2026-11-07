// Your own types: plain structs need nothing; others define cp_read / cp_write next to the type.
#include "cp/io/io.hpp"
#include <vector>
using namespace cp;

struct edge {                                 // a plain struct: its fields are read and written in order
    int u, v;
    long long w;
};

class fraction {                              // not a plain struct: say how to read and write it
public:
    long long num = 0, den = 1;
};
void cp_read(io::reader& r, fraction& f) { r.read(f.num, f.den); }
void cp_write(io::writer& w, fraction const& f) {
    w.write(f.num);
    w.put('/');
    w.write(f.den);
}

int main() {
    std::vector<edge> edges(*io::read<int>());
    io::read(edges);
    fraction f;
    io::read(f);
    for (auto const& e : edges) io::writeln(e);  // u v w
    io::writeln(f);                              // num/den
}
