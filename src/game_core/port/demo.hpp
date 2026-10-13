// PORT-SIDE FILE. No original source path for this module was recovered from the ELF, so
// its name and location are this project's, not the original project's (TASK-0020). Do not
// read this path as evidence of where the original kept this code.
#pragma once
#include <cstdint>
namespace port::core {
// Synthetic infrastructure contract. No original-game timing or provenance.
struct DemoState { std::uint64_t frame = 0; std::int64_t position = 0; };
inline void demo_tick(DemoState &state, int axis) noexcept {
    ++state.frame;
    state.position += axis < 0 ? -1 : axis > 0 ? 1 : 0;
}
}
