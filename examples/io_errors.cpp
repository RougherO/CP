// A read returns a result: success, or why it failed.
#include "cp/io/io.hpp"
using namespace cp;

int main() {
    int x = 0;
    if (auto r = io::read(x); !r) {
        switch (r.error()) {
            case io::read_error::end_of_input: io::writeln("no input"); break;
            case io::read_error::invalid:      io::writeln("not a number:", io::in.token()); break;
            case io::read_error::out_of_range: io::writeln("too big:", io::in.token()); break;
        }
        return 0;
    }
    auto y = io::read<long long>();           // a value or an error
    io::writeln(x + y.value_or(0));
}
