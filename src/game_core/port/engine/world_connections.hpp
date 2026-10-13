// PORT-SIDE FILE. No original source path for this module was recovered from the ELF, so
// its name and location are this project's, not the original project's (TASK-0020). Do not
// read this path as evidence of where the original kept this code.
#pragma once
#include <cstdint>
#include <vector>

namespace hp2::engine {
// Evidence-backed connection metadata. Not an executable fixed-size entity loop.
// A future World handler can bind concrete wrapper targets through ModuleRegistry.
struct WorldConnection {
    std::uint32_t caller, pc, direct_target;
    const char* slot;
    const char* contract;
    const char* confidence;
    const char* evidence;
};
const std::vector<WorldConnection>& world_connections();
} // namespace hp2::engine
