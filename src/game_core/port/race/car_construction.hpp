#pragma once
// Port-only bindings of actual shared World resources to original Car callees.
#include "game_core/port/driving/projections.hpp"
#include "game_core/indep/src/track.hpp"
#include "game_core/port/track/route_data.hpp"
#include "game_core/port/track/topology_resources.hpp"
#include "game_core/port/race/collision_resources.hpp"
#include "game_core/port/race/character_resources.hpp"
#include "game_core/port/race/debug_transport.hpp"
#include <stdexcept>
#include <optional>
namespace port::race {
class CarSoundResources;
struct CarConstructionServices {
    const ::port::track::RouteData* routes=nullptr;
    const ::port::track::TopologyResources* topology=nullptr;
    const CollisionResources* collisions=nullptr;
    const CharacterResources* characters=nullptr;
    CarSoundResources* audio=nullptr;
    MonitorSyscall monitor;
    // Explicit PC policy for the original uninitialized local bVector3 padding
    // lane. Original XYZ stores are known; the stack's fourth lane is unknown.
    std::optional<std::uint32_t> port_stack_vector_lane;
};
// Binds the original explicit TopologyCoordinate argument used by Car placement.
// Every query uses the real shared cache/refcounts and loaded tree; no plane or
// independent host topology substitutes for a missing original resource owner.
class SharedTopologyContact final : public ::port::driving::ContactProvider {
    ::port::driving::Memory& memory_;
    std::uint32_t coordinate_;
public:
    SharedTopologyContact(::port::driving::Memory& memory,std::uint32_t coordinate)
        :memory_(memory),coordinate_(coordinate){}
    float GetElevation(::port::driving::Memory& m,std::uint32_t owner,std::uint32_t point,
                       std::uint32_t material,std::uint32_t plane,std::uint32_t found)override{
        if(&m!=&memory_ || (owner&&owner!=coordinate_))throw std::logic_error("Car topology binding escaped its shared coordinate");
        const auto p=m.vector(point);int terrain=0;bool valid=false;
        auto normal=plane?m.vector(plane):std::array<float,4>{};
        const auto height=hp2::debug::TopologyCoordinate(m,coordinate_).GetElevation(
            {p[0],p[1],p[2]},material?&terrain:nullptr,plane?&normal:nullptr,found?&valid:nullptr);
        if(material)m.word(material,static_cast<std::uint32_t>(terrain));
        if(plane)m.vector(plane,normal);
        // Original00151CC0 uses full SW stores even though the formal type is bool*.
        if(found)m.word(found,valid?1u:0u);
        return height;
    }
};
}
