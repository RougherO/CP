// util::result: a value or an error, like Rust's Result; success and failure build one.
#include "cp/io/io.hpp"
#include "cp/util/util.hpp"
#include <string>
using namespace cp;

auto safe_div(long long a, long long b) -> util::result<long long, std::string> {
    if (b == 0) return util::failure(std::string("division by zero"));
    return util::success(a / b);              // or just `return a / b;`
}

int main() {
    auto q = safe_div(7, 2);
    auto z = safe_div(1, 0);
    if (q) io::writeln(*q);                   // 3
    if (!z) io::writeln(z.error());           // division by zero
    io::writeln(z.value_or(-1));              // -1
}
