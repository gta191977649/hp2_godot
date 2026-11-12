#pragma once
// Port-only bindings for the real Debug PhysicsCar construction path.
#include "game_core/port/driving/object_view.hpp"
#include "game_core/port/race/snapshot_services.hpp"
#include <functional>
#include <optional>
namespace port::driving {class ContactProvider;}
namespace port::race {
struct CarResetServices;
struct PhysicsConstructionServices {
    // Original bGetTicker0024A340 Count register, at scalar00326254 ticks/second.
    std::function<std::uint32_t()> ticker;
    ::port::driving::ContactProvider* contact=nullptr;
    // E-0110-F15: original deferred Car.Move reset uses frame-owned context.
    const CarResetServices* car_reset=nullptr;
    std::optional<std::uint32_t> port_stack_vector_lane;
    std::optional<std::uint32_t> cop_speedtrap_velocity_w;
    std::optional<std::uint32_t> cop_desired_velocity_w; //0019FF20 independent source local vel.pad.
    std::optional<std::uint32_t> cop_roadblock_velocity_w; //00229208 independent source zero-velocity.pad.
    std::optional<std::uint32_t> cop_bump_arm_w; //00229BE0 independent source impulse arm.pad.
    std::optional<std::uint32_t> cop_target_point_w; //0022AE40 independent local position_to_drive.pad.
    std::optional<std::uint32_t> cop_mover_ground_normal_w;
    std::optional<std::uint32_t> cop_mover_invalid_ground_normal_w;
    std::optional<std::uint32_t> cop_mover_airborne_angular_w;
    std::optional<std::uint32_t> mellow_jump_up_w; // E-0110-M01: independent original stacksp+3C operand.
    std::optional<std::uint32_t> average_air_position_w; //00135B88 sp+4C, independent unknown initial accumulator.
    std::optional<std::uint32_t> average_air_acceleration_w; //00135B88 sp+6C, independent unknown acceleration W.
    bool disable_jump_camera_presentation=false; //AA01: prediction/timing mandatory, no fabricated zone/time-scale acceptance.
    // TASK-0111 user-authorized PC presentation policy. Wheel forces, contact
    // and movement remain mandatory. Strict reconstruction keeps this true.
    bool enable_skid_trails=true;
    // E-0110-F13: disable only reviewed collision_sound.PlaySmokeable001E1B68.
    bool disable_smokeable_collision_sound=false;
    // E-0110-F17: preserve lap selection/RNG; disable only SPCH announcement.
    bool disable_race_lap_speech=false;
    // E-0100-PA01: preserve pursuit selector/state/speaker/RNG; omit only
    // reviewed speech event submission and NIS resource loading before entry.
    std::optional<std::uint32_t> pursuit_add_point_w; //00216DD8 independent localW, PU02.
    std::optional<std::uint32_t> pursuit_set_point_w; //00218618 independent localW, PU02.
    bool disable_pursuit_speech_emission=false;
    bool disable_pursuit_nis_asset_load=false;
};
}
namespace hp2::debug {
std::uint8_t GetJoystickType(::port::driving::Memory&,int); //Debug00153F20, E-0099-W42.
bool IsJoystickTypeWheel(::port::driving::Memory&,int); //Debug0011D720.
// E-0099-W33, NFSHP2.ELF Debug SHA256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
// Original indep/src/physicscar.cpp; explicit operations, no host lifetime/ABI claim.
class PhysicsCar : public ::port::driving::ObjectView {
public:
    using ObjectView::ObjectView;
    void DoSnapshot(std::uint32_t snapshot,const ::port::race::SnapshotServices& ={}); //Debug00137410, E-0099-W74.
    void Sub_001370C0(std::uint32_t config,std::uint32_t driver,std::uint32_t car,
                     ::port::race::PhysicsConstructionServices&);
    void Sub_001376C8(int);
    void ResolveForces(::port::race::PhysicsConstructionServices&); //00138D90
    float Move(float,::port::race::PhysicsConstructionServices&); //00137D18, E-0099-W42.
    void ComputeAuxiliaries(::port::race::PhysicsConstructionServices&); //00137748, shared binding.
};
}
