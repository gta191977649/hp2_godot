// TASK-0042 E-0042-S001: exact Debug source marker(s): ../indep/inc/World.hpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/driving/static_collisions.hpp"
#include "game_core/port/race/debug_transport.hpp"
#include "game_core/port/race/snapshot_services.hpp"
namespace port::race {struct CarConstructionServices;struct AiDriveTargetServices;struct WorldConstructionServices;struct PhysicsConstructionServices;struct WorldFrameServices;}

namespace hp2::debug {
// TASK-0099 E-0099-W01. Shared logical-memory projection; this host constructor
// does NOT stand in for original World0014CA28 construction or its dependencies.
class World {
public:
    World(::port::driving::Memory& memory,std::uint32_t address):memory_(memory),address_(address){}
    // Debug0014CA28, E-0099-W06. Explicit original construction, separate from
    // the host view above. Required resource callees must be supplied in full.
    // A throw leaves original partial construction visible in candidate Memory;
    // the session must own/discard that candidate rather than publish it.
    void Sub_0014CA28(const ::port::race::WorldConstructionServices&);
    // Debug0014CC80, E-0099-W18/W35. Original teardown including AI pointer
    // nodes and owned Cars; active collision/OBB/callee gaps remain explicit.
    void Sub_0014CC80(int in_chrg);
    void DoTimestep(float,::port::race::WorldFrameServices&); //0014D9A8 E-0099-W67; actual shared frame.
    void DoTimestepMove(float timestep,::port::race::PhysicsConstructionServices&); //0014D588, E-0099-W38
    void DoTimestepCollisions(float timestep,::port::race::PhysicsConstructionServices&); //0014D630, E-0099-W38
    void FlushCollisions(); //0014E260, E-0099-W38; instruction-confirmed empty
    void VerifyDebugChecksums(const ::port::race::MonitorSyscall& ={}); //0014DFD0, E-0099-W49.
    std::uint16_t CalculateDebugChecksum(); //0014DF10, E-0099-W48.
    // Debug0014E268, E-0099-W91. Appends only a genuinely placed Car.
    void AddTrafficCar(std::uint32_t,const ::port::race::CarConstructionServices&,::port::race::PhysicsConstructionServices&,const ::port::race::AiDriveTargetServices&);
    void BuildCarTable();
    std::uint32_t GetCarByNumber(int) const;
    void ResetTimeScale();
    void DoPointerSnapshot(std::uint32_t snapshot,std::uint32_t ppcar); //0014D240 E-0099-W76.
    void DoCarPointerSnapshot(std::uint32_t ppcar,std::uint32_t snapshot){DoPointerSnapshot(snapshot,ppcar);} //00130A58 original World.hpp inline.
    void DoSnapshot(std::uint32_t,const ::port::race::SnapshotServices& = {}); //0014CE40 E-0099-W76.

    void SetWorldPaused(float,std::uint32_t original_callback,int param); //0014D348, E-0099-W75.
    void UpdateWorldPaused(); //0014D368, null-callback timer domain.
    int IsWorldPaused()const; //0014D3D0.
    float GetTimestep(const std::function<std::uint32_t()>& ticker={}); //0014D408; primitive Count reader only.

    std::uint32_t Random(int); //Debug0014E208, E-0099-W47.
    float Random(float); //Debug0014E230, E-0099-W37; actual shared seed.
    void IncrementCollisionPredictionID(); //Debug0014E250, E-0099-W37.
    //Debug0014E0D0/0014E128, E-0099-W44. Actual cached two-player positions;
    // arrays are port decoded operands, not an original PS2 ABI.
    float GetDistanceToClosestPlayer(const std::array<float,2>&)const;
    float GetDistanceToClosestPlayer(const std::array<float,3>&)const;
    float GetDATToClosestPlayer(float)const; //0014E188, E-0099-W67; signed nearest wrapped DAT, second wins ties.
private:
    ::port::driving::Memory& memory_;
    std::uint32_t address_;
};
int CarListCheckFlip(::port::driving::Memory&,std::uint32_t before,std::uint32_t after); //0014D2E8 E-0099-W67.
void Sort(::port::driving::Memory&,std::uint32_t list,std::uint32_t comparator); //0014E5B0, actual World list instantiation.
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): World::DoTimestep() [0x002AFD18]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFD30]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFD58]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFD78]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFDA0]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFDC8]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFDE8]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFE10]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFE30]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFE58]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFE80]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFEB0]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFED8]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFF00]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFF28]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFF50]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFF78]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFFA0]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFFC8]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002AFFF0]
// Diagnostic text (not a callable signature): World::DoTimestep [0x002B0020]
namespace hp2::recovered {
class World {
public:
    // Debug0014D630; E-0048-T06. Single dynamic car / immutable track branch.
    static void DoTimestepCollisions(::port::driving::Memory&,::port::driving::StaticCollisions&,std::uint32_t,std::uint32_t,const ::port::track::Topology&,::port::driving::CollisionMotion&,const std::function<void(float)>&,const std::function<void()>&);
    // No member address is confirmed; no speculative members are declared.

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x0014D338; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0014d338.md.
    // Original Debug reference: World::BuildCarTable(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void BuildCarTable();
    // ELF 0x0014D508; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0014d508.md.
    // Original Debug reference: World::GetCarByNumber(int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetCarByNumber();
    // ELF 0x0014D638; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0014d638.md.
    // Original Debug reference: World::ResetTimeScale(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ResetTimeScale();
    // ELF 0x0014E3E8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0014e3e8.md.
    // Original Debug reference: World::GetDistanceToClosestPlayer(bVector2 const *); multiple original overloads cannot share a void/no-argument placeholder.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_0014E3E8();
    // ELF 0x0014E440; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0014e440.md.
    // Original Debug reference: World::GetDistanceToClosestPlayer(bVector3 const *); multiple original overloads cannot share a void/no-argument placeholder.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_0014E440();
};
} // namespace hp2::recovered
