#include "game_core/port/scaffold.hpp"
#include <iostream>
#include <limits>

int main() {
    using port::core::scaffold_add;
    const auto max = std::numeric_limits<std::int32_t>::max();
    const auto min = std::numeric_limits<std::int32_t>::min();
    if (scaffold_add(20, 22) != 42 || scaffold_add(-7, 7) != 0 ||
        scaffold_add(0, 0) != 0 || scaffold_add(max, max) != 4294967294LL ||
        scaffold_add(min, min) != -4294967296LL) {
        std::cerr << "Scaffold arithmetic contract failed\n";
        return 1;
    }
    std::cout << "Scaffold arithmetic passed; no game behavior tested\n";
    return 0;
}
