// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/driving/memory.hpp"
#include <functional>
#include <optional>
namespace port::race {
struct CarConstructionServices;struct PhysicsConstructionServices;struct AiDriveTargetServices;struct CarResetServices;
// Original route-manager lookup: nullopt means an actually absent route, not an unbound provider.
using RoutePointCount=std::function<std::optional<std::uint32_t>(std::uint32_t)>;
}
namespace hp2::debug {
struct NearestRoutePointInfo;
class TrafficTeleporter {
public:
    // Port view of an already constructed original object; no source ctor writes.
    TrafficTeleporter(::port::driving::Memory& memory,std::uint32_t address):memory_(memory),address_(address){}
    TrafficTeleporter(::port::driving::Memory&,std::uint32_t,const ::port::race::RoutePointCount&);
    void FillInTotals(std::uint32_t array,std::uint32_t route_start,const ::port::race::RoutePointCount&);
    void SetThinkTime();
    // Debug0021D1D8..0021DAE0, E-0099-W91; original resource-consuming placement.
    std::uint32_t PlaceNewCar(std::uint32_t,const ::port::race::CarConstructionServices&,::port::race::PhysicsConstructionServices&,const ::port::race::AiDriveTargetServices&);
    bool ForwardsOrBackwards();
    void GenerateRandomPoint(NearestRoutePointInfo&,bool);
    bool CanPlaceCarHere(const NearestRoutePointInfo&,std::uint32_t,bool);
    bool TooCloseToAnotherCar(float,std::uint32_t,bool);
    int PickRandomSkin(std::uint32_t);
    void DoSnapshot(std::uint32_t); //0021D078 E-0099-W76.
    void DoTimestep(float,const ::port::race::CarResetServices* =nullptr); //Debug0021D0D8.
    void TryToTeleportCar(std::uint32_t,const ::port::race::CarResetServices&); //0021D380,E-0110-T01.
    std::uint32_t RandomPlayerCar(); //0021DAA8.
    void SwitchSkin(std::uint32_t); //0021DB30.
    bool CheckIsVisible(const std::array<float,2>&); //0021D678.
    bool CheckIsVisible(float); //0021D6B8.
private:
    ::port::driving::Memory& memory_;
    std::uint32_t address_;
};
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): TrafficTeleporter::DoTimestep() [0x002BCDC0]
namespace hp2::recovered {
class TrafficTeleporter {
public:
    // No member address is confirmed; no speculative members are declared.
};
} // namespace hp2::recovered
