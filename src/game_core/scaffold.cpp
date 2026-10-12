#include "game_core/scaffold.hpp"

namespace port::core {
std::int64_t scaffold_add(std::int32_t left, std::int32_t right) noexcept {
    return static_cast<std::int64_t>(left) + static_cast<std::int64_t>(right);
}
}
