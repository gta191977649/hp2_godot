#pragma once
#include "game_core/indep/src/volumetree.hpp"
#include "game_core/port/driving/memory.hpp"
// E-0048-A06: port companion to original src\ecstasy\elighte.cpp.
// Explicit host values and ownership, never original PS2 ABI storage.
namespace hp2::recovered {
struct eLight {
    std::uint8_t Type{},AttenuationType{};
    std::uint32_t Colour{};
    float PositionX{},PositionY{},PositionZ{},Size{},Intensity{},FarStart{};
};
struct eLightQuery {
    std::array<const eLight*,10> Lights{};
    std::array<float,10> Influences{};
    float AmbientR{},AmbientG{},AmbientB{},LightScale{};
};
}

namespace hp2::debug {
// E-0098-R02. Decoded typed selection lights, no PS2 ABI/layout claim.
struct SelectCarLight {std::array<float,3> Position{},Colour{};};
struct SelectCarLights {SelectCarLight Light0,Light1,Light2,LightA;};
}
namespace port::selection {
struct LightingDefaults {hp2::debug::SelectCarLights normal,silouette;};
// Scoped selected fields of Debug_GLOBAL__I_LightTypeStrings00117708.
LightingDefaults load_lighting_defaults(::port::driving::Memory&);
}
namespace hp2::debug {
void SetSelectCarLighting(std::array<SelectCarLights,2>&,int view_id,float intensity_multiply,int silouette,
                         const ::port::selection::LightingDefaults&);
// Selector branch only; original view/part inverse rotations and VU35C..362 packet.
std::array<float,28> elSetupLightContext(const SelectCarLights&,const std::array<float,16>& world_view,
                                      const std::array<float,16>& local_world);
}
namespace port::track {
struct LightPack { hp2::recovered::vAABBTree tree;std::vector<hp2::recovered::eLight> lights; };
}
namespace hp2::recovered {
// Debug00115780; Retail00115370 bounded correspondence, E-0048-A06.
int QueryLightDatabase(eLightQuery&,const std::vector<::port::track::LightPack>&,
    const std::array<float,3>& position);
// Retail00115978 / Debug00115D88; E-0048-A06. World identity in-game slice
// only, using proven initialized Retail directional constants. Returns VU35C..362.
std::array<float,28> elSetupLightContext(const std::vector<::port::track::LightPack>&);
// TASK-0051. Original in-game per-instance branch, native local->world matrix.
std::array<float,28> elSetupLightContext(const std::vector<::port::track::LightPack>&,
    const std::array<float,16>& local_world);
}
