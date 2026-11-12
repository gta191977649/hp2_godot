// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/track/route_data.hpp"
#include <functional>
#include "game_core/port/driving/object_view.hpp"
#include "game_core/indep/inc/TrackRoute.hpp"
namespace hp2::debug {
bool HasSignposts(::port::driving::Memory&); //Debug001C0088, E-0099-W120.
class TrackRoutePoint {
    ::port::driving::Memory& memory_;std::uint32_t address_;
public:
    TrackRoutePoint(::port::driving::Memory& memory,std::uint32_t address):memory_(memory),address_(address){}
    // Debug001BE5C0/001BE620/001BE738; E-0099-W14.
    void SwapPointData(std::uint32_t);
    void ReversePoint(float track_length);
    void ReverseTrafficRoutePoint(float track_length);
    // Debug001BE440/001BE750, E-0099-W17.
    std::array<float,2> GetPosition(float,float)const;
    // Debug001BE4B8, E-0099-W37. Recovered XYZ; original temporary padding
    // is unknown, so the port output lane is an explicit execution operand.
    void GetPosition(std::uint32_t output,float distance,float lateral,std::uint32_t port_stack_vector_lane)const;
    float GetDistanceAway(const std::array<float,2>&,float,float track_length)const;
    float GetAmountOutOfBounds(float lateral)const; //Debug0023EF70, TrackRoute.hpp emitted drivingchoice.cpp, W37.
    std::uint32_t GetSignpost()const; //Debug001BE5A8, E-0099-W47.
};
class TrackRoute {
    ::port::driving::Memory& memory_;std::uint32_t address_;
public:
    TrackRoute(::port::driving::Memory& memory,std::uint32_t address):memory_(memory),address_(address){}
    // Debug001BF3E8/001BF650/001BF6B0; E-0099-W14.
    void ReverseRoute(float track_length);
    void ReverseTrafficRoute(float track_length);
    void SetFirstAndLast(const ::port::track::RouteData&);
    // Debug001BEBF8, E-0099-W17. Original bbox/flags/skip traversal.
    std::uint32_t FindClosestPoint(const std::array<float,2>&,float,int,float,float track_length)const;
    bool DistanceIsWithinRoute(float)const; //Debug001BF830, E-0099-W37.
    std::uint32_t FindClosestPointAhead(const std::array<float,2>&,std::uint32_t initial)const; //Debug001BEDE0, W47.
};
class Signpost {
public:
    static std::uint32_t GetNewRoute(::port::driving::Memory&,std::uint32_t); //Debug001BE0C0, W47.
    static std::uint32_t GetNewRoutePoint(::port::driving::Memory&,std::uint32_t); //Debug001BE0E8.
    // Debug001BE280; E-0099-W24. Original distance scan, wraps to index0.
    static int GetIndex(::port::driving::Memory&,const ::port::track::RouteData&,float);
    // Debug001BE128/001BE1F0; E-0099-W14. Shared manager/sign globals supplied
    // explicitly through the port-owned versioned resource carrier.
    static void ReverseSigns(::port::driving::Memory&,const ::port::track::RouteData&);
    static void SetDistances(::port::driving::Memory&,const ::port::track::RouteData&);
};
// Debug001C0098/001C02D8/001C02F0; E-0099-W14.
void HeapSort(::port::driving::Memory&,std::uint32_t,int,int,const std::function<float(std::uint32_t,std::uint32_t)>&);
float CompareSignposts(::port::driving::Memory&,std::uint32_t,std::uint32_t);
void InitTrackRoutes(::port::driving::Memory&,::port::track::RouteData&,bool forwards);
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class TrackRouteManager {
public:
    // ELF 0x001C28A0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c28a0.md.
    // Original Debug reference: TrackRouteManager::AddRouteToRightList(TrackRoute *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void AddRouteToRightList();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class TrackRoutePoint {
public:
    // ELF 0x001C1340; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c1340.md.
    // Original Debug reference: TrackRoutePoint::TrackRoutePoint(void); constructor/destructor entry is not a native C++ lifetime operation.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001C1340();
    // ELF 0x001C1408; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c1408.md.
    // Original Debug reference: TrackRoutePoint::GetPosition(bVector3 *, float, float); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetPosition();
    // ELF 0x001C1490; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c1490.md.
    // Original Debug reference: TrackRoutePoint::SetWidth(float, float); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void SetWidth();
    // ELF 0x002455B0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002455b0.md.
    // Original Debug reference: TrackRoutePoint::GetAmountOutOfBounds(float); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetAmountOutOfBounds();
};
} // namespace hp2::recovered
