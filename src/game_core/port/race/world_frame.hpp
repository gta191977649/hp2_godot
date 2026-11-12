#pragma once
// TASK-0099 E-0099-W67. Port-only typed resources and primitive bindings for
// the actual World frame. No game-function replacement callbacks are accepted.
#include "ai_drive_target.hpp"
#include "physics_construction.hpp"
#include "game_core/port/driving/player_services.hpp"
#include "game_core/port/track/acidfx_services.hpp"
#include "debug_transport.hpp"
namespace port::race {
struct WorldFrameServices {
 const AiDriveTargetServices& ai;
 const ::port::track::RouteData& routes;
 PhysicsConstructionServices& physics;
 ::port::driving::PlayerServices* player=nullptr;
 ::port::track::AcidEffectServices acid;
 MonitorSyscall monitor;
 // Three independently unwritten original stack operands; no implicit zero.
 std::optional<std::uint32_t> event_disc_w,terrain_disc_w,camera_shake_w;
 std::optional<std::uint32_t> cop_teleport_position_w;
 std::optional<std::uint32_t> car_reset_route_newpos_w,car_reset_velocity_w;
};
}
