// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/track/route_data.hpp"

namespace port::race {struct AiDriveTargetServices;}
namespace hp2::debug {
// E-0099-W07. Debug memory view; original arrays/lists share the World Memory.
// This is independent of the existing Retail placeholder and native object ABI.
class tTrackGraph {
    port::driving::Memory& memory_;std::uint32_t address_;
public:
    tTrackGraph(port::driving::Memory& memory,std::uint32_t address):memory_(memory),address_(address){}
    // Debug00247218 /00247608; E-0099-W07. Actual graph/pool construction/cleanup.
    void Sub_00247218(const port::track::RouteData&);
    void Sub_00247608(int in_chrg);
    // Debug002478E0/00247990/00247A20; E-0099-W07.
    void AddVertex(std::uint32_t route,std::uint32_t point,const port::track::RouteData&);
    int GetVertexNumber(int route,int point)const;
    int AddEdge(int vertex,bool take_branch,bool cloaked,const port::track::RouteData&);
    // Debug00247878/00247C30/00248148/00248150/00248168; E-0099-W07.
    void DoSnapshot(std::uint32_t); //002477D8 E-0099-W76.
    void UncloakAllEdges();std::uint32_t FindEdgeByRouteAndPoint(int,int)const;
    std::uint32_t FindSpeedTrap(std::uint32_t,float,float,bool,const ::port::race::AiDriveTargetServices&)const; //00247F00,E-0110-F07.
    bool HasCloakedEdges()const;
    std::uint32_t GetVertex(int)const;std::uint32_t GetEdge(int)const;
};
// Debug002480D0 /00248100; E-0099-W07. pTrackGraph is published only after ctor.
void InitTrackGraph(port::driving::Memory&,const port::track::RouteData&);
void CleanupTrackGraph(port::driving::Memory&);
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class tTrackGraph {
public:
    // ELF 0x0024E4D0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0024e4d0.md.
    // Original Debug reference: tTrackGraph::GetVertexNumber(int, int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetVertexNumber();
};
} // namespace hp2::recovered
