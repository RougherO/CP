// Type traits that work from C++17 on.
#include "cp/io/io.hpp"
#include "cp/traits/traits.hpp"
#include <map>
#include <vector>
using namespace cp;

template <class T>
using size_of = decltype(std::declval<T&>().size());

enum class colour : unsigned char { red = 1, green = 2 };

int main() {
    io::writeln(traits::is_detected_v<size_of, std::vector<int>>, traits::is_detected_v<size_of, int>);   // 1 0
    io::writeln(traits::is_specialization_of_v<std::map<int, int>, std::map>);                             // 1
    io::writeln(traits::to_underlying(colour::green));                                                    // 2
}
