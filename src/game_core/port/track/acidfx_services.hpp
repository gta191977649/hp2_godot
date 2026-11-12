#pragma once
// TASK-0099 E-0099-W53. Port-only primitive clock and explicitly chosen original
// unwritten stack-lane policies. None of these values is original game behavior.
#include <array>
#include <cstdint>
#include <functional>
#include <optional>
namespace port::track {
struct AcidEffectServices {
 std::function<std::uint32_t()> ticker; // raw original Count units, no default0
 std::optional<std::uint32_t> port_stack_vector_lane; // disc pdir.W
 std::optional<std::array<std::uint32_t,8>> port_extra_control_lanes; // frontend rows Z/W
};
}
