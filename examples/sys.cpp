// What the CPU running the program supports.
#include "cp/io/io.hpp"
#include "cp/sys/sys.hpp"
using namespace cp;

int main() {
    io::writeln(sys::cpu_supports(sys::cpu_feature::avx2), sys::cpu_supports(sys::cpu_feature::popcnt));
}
