// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\carcontroller.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring class with adopted retail correspondence; evidence: E-0042-S001.
// Placement: TASK-0042 E-0042-S001; exact Debug definition source/header markers where recorded; unmatched companion remains port-side; retail ABI unconfirmed.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/inc/CarController.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001C0100; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001c0100.md.
void CarController::SetToPhysicsMode() {
    hp2::not_implemented("0x001C0100", "CarController::SetToPhysicsMode (signature and full body unresolved)");
}

// ELF 0x001C0140; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001c0140.md.
void CarController::ForceBoost() {
    hp2::not_implemented("0x001C0140", "CarController::ForceBoost (signature and full body unresolved)");
}

// ELF 0x001C0160; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001c0160.md.
void CarController::ReleaseBoost() {
    hp2::not_implemented("0x001C0160", "CarController::ReleaseBoost (signature and full body unresolved)");
}

// ELF 0x001C0198; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001c0198.md.
void CarController::DebugDisplay() {
    hp2::not_implemented("0x001C0198", "CarController::DebugDisplay (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001C00A8; E-0042-U143; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001c00a8.md.
void CarController::Sub_001C00A8() {
    hp2::not_implemented("0x001C00A8", "CarController::Sub_001C00A8 (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::debug {
// Debug001BD1A8, E-0099-W21. Original does not initialize Driver+4.
void CarController::Sub_001BD1A8(std::uint32_t car){m_.word(address_,car);m_.word(address_+8,0x2aefc8);}
// Debug001BD1C0, E-0099-W21.
void CarController::Sub_001BD1C0(int in_chrg){m_.word(address_+8,0x2aefc8);if(in_chrg&1)m_.release(address_);}
}

#include "game_core/indep/inc/Car.hpp"
namespace hp2::debug {
// Debug001BD218, E-0099-W36. Original collision operand is unused by base.
void CarController::SetToPhysicsMode(std::uint32_t,::port::race::PhysicsConstructionServices& services){
 const auto car=m_.word(address_),mode=m_.word(car+0x24);
 if(mode!=1&&mode!=4)hp2::recovered::Car(m_,car).SetMovementMode(m_.word(car+0x18)?1:4,services);
}
}

#include "game_core/indep/inc/Replay.hpp"
#include "game_core/indep/inc/CarDriver.hpp"
#include "game_core/port/race/control_construction.hpp"
namespace hp2::debug {
// Debug001BD1E8; E-0099-W74; replay-frame/001bd1e8.json/type-CarDriver__vtable.json.
void CarController::DoSnapshot(std::uint32_t snapshot){
 const auto driver=m_.word(address_+4),entry=m_.word(driver+0x22c)+0x18;
 const auto adjusted=driver+static_cast<std::uint32_t>(m_.signed_half(entry));const auto target=m_.word(entry+4);
 if(target==0x130aa0)CarDriver(m_,adjusted).DoSnapshot(snapshot);
 else if(target==0x1398b8)PsychoDriver(m_,adjusted).DoSnapshot(snapshot);
 else if(target==0x13acc0)RealDriver(m_,adjusted).DoSnapshot(snapshot);
 else hp2::not_implemented("0x001BD1E8","CarDriver snapshot virtual target");
}
}

namespace hp2::debug {
// Debug001BD278,E-0099-W121; actual carcontroller.cpp. Both source null tests retained.
void CarController::ReleaseBoost(){const auto car=m_.word(address_);if(car&&car!=0xffffffc0u)m_.scalar(car+0x3e0,1);}
}
