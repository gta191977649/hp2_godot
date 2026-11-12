#pragma once
// TASK-0099 E-0099-W122. Port-only resource/unknown-storage operands. These
// independent original stack words are not inferred from each other or from
// PhysicsConstructionServices' separate mover-construction stack operand.
#include <cstdint>
#include <optional>
namespace port::track {struct RouteData;}
namespace port::race {
struct PhysicsConstructionServices;
struct AiDriveTargetServices;
struct CarResetServices {
 const ::port::track::RouteData* routes=nullptr;
 PhysicsConstructionServices* physics=nullptr;
 const AiDriveTargetServices* ai=nullptr;
 std::optional<std::uint32_t> route_newpos_w;
 std::optional<std::uint32_t> reset_velocity_w;
 std::optional<std::uint32_t> cop_teleport_position_w;
};
}
