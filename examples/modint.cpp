// Arithmetic modulo a prime or any other modulus, and the constants that come with it.
#include "cp/io/io.hpp"
#include "cp/num/num.hpp"
using namespace cp;
using cp::num::consts::inf, cp::num::consts::mod1e9_7, cp::num::consts::pi, cp::num::consts::tau;
using mint = num::modint998;                  // modulo 998244353; num::modint<M> for any other M

int main() {
    mint a(-1), b(10);                        // explicit: a is 998244352
    io::writeln(a + b, a * b, b / 3 * 3 == b, b.pow(-1) * b);   // 9 998244343 1 1

    mint x;
    io::read(x);                              // any integer, reduced: -5 reads as 998244348
    io::writeln(x, x.inv() * x);

    num::dyn_modint<>::set_mod(*io::read<int>());   // a modulus from the input
    num::dyn_modint<> y(100);
    io::writeln(y, y * y);

    io::writeln(inf, mod1e9_7, pi, tau);
}
