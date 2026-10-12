#include "game_core/demo.hpp"
#include <iostream>
#include <string>
int main(int argc, char **argv) {
    port::core::DemoState state;
    for (int i = 1; i < argc; ++i) {
        const std::string axis(argv[i]);
        if (axis != "-1" && axis != "0" && axis != "1") return 2;
        port::core::demo_tick(state, std::stoi(axis));
    }
    std::cout << "{\"frame\":" << state.frame << ",\"position\":" << state.position << "}\n";
}
