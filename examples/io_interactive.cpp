// An interactive problem: guess a number in [1, 10^6]; the judge answers 1 if the secret is at most the query.
#include "cp/io/io.hpp"
using namespace cp;

int main() {
    int lo = 1, hi = 1'000'000;
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        io::writeln('?', mid);                // no flush needed: reading flushes io::out first
        if (*io::read<int>() == 1) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
    io::writeln('!', lo);
}
