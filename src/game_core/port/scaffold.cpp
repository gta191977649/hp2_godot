// PORT-SIDE FILE. No original source path for this module was recovered from the ELF, so
// its name and location are this project's, not the original project's (TASK-0020). Do not
// read this path as evidence of where the original kept this code.
#include "game_core/port/scaffold.hpp"

namespace port::core {
std::int64_t scaffold_add(std::int32_t left, std::int32_t right) noexcept {
    return static_cast<std::int64_t>(left) + static_cast<std::int64_t>(right);
}
}
