// TASK-0042 E-0042-S001: exact Debug source marker(s): ../indep/inc/World.hpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/driving/static_collisions.hpp"

namespace hp2::debug {
// TASK-0099 E-0099-W01. Shared logical-memory projection; this host constructor
// does NOT stand in for original World0014CA28 construction or its dependencies.
class World {
public:
    World(::port::driving::Memory& memory,std::uint32_t address):memory_(memory),address_(address){}
    void BuildCarTable();
    std::uint32_t GetCarByNumber(int) const;
    void ResetTimeScale();
private:
    ::port::driving::Memory& memory_;
    std::uint32_t address_;
};
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
    static void DoTimestepCollisions(port::driving::Memory&,port::driving::StaticCollisions&,std::uint32_t,std::uint32_t,const port::track::Topology&,port::driving::CollisionMotion&,const std::function<void(float)>&,const std::function<void()>&);
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
