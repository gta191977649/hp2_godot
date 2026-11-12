// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\munitions.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/MunitionEffectsHandler.hpp"

#include "game_core/indep/src/munitions.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/port/race/munitions.hpp"
#include "game_core/inc/bSlotPool.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/port/track/emitter_bindings.hpp"
#include "game_core/src/bList.hpp"
#include "game_core/indep/inc/AcidFX.hpp"

namespace hp2::debug {
// Debug001BCB50, E-0099-W109. The original timestep parameter is unused.
// Save next before source removal/free; active State1 emitters stay borrowed.
void MunitionEffectsHandler::DoTimestep(float){for(auto node=m_.word(address_);node!=address_;){const auto emitter=m_.word(node+8),next=m_.word(node);if(m_.word(emitter+0x10)!=1){bPList<AcidEmitter>(m_,address_).Remove(node);AcidEffectSystem(m_,0x2e6f40).RemoveAutoEmitter(emitter);if(emitter)AcidEmitter(m_,emitter).Sub_00124FA0(3);}node=next;}}
}

namespace hp2::debug {
// Debug001BCDC0, E-0099-W09. Only the original static initializer resets
// these lists; InitMunitions does not discard existing emitter ownership.
void Sub_001BCDC0(::port::driving::Memory& memory){
    for(auto head:{0x328160u,0x328168u,0x328170u})memory.word(head+4,head);
    for(auto head:{0x328160u,0x328168u,0x328170u})memory.word(head,head);
}
// Debug001BBDB0, E-0099-W09. Exact pool/global and original creator identity.
void SmokeBomb::InitSmokeBomb(::port::driving::Memory& memory,const ::port::race::EmitterGroupLookup& emitters){
    memory.word(0x328154,bNewSlotPool(memory,0x480,14,0x2aea40,0x10));
    RegisterSmokeableCreator(memory,ST_SMOKEBOMB,0x1bb6a0);
    const auto hash=hp2::frontend::bStringHash("ROCKETEXPLD");
    if(!emitters)hp2::not_implemented("0x00127438","original munition explosion emitter-group lookup is not bound");
    emitters(memory,0x328168,hash);
}
// Debug001BC560, E-0099-W09.
void Rocket::InitRocket(::port::driving::Memory& memory,const ::port::race::EmitterGroupLookup& emitters){
    memory.word(0x328150,bNewSlotPool(memory,0x490,3,0x2aeaa0,0x10));
    RegisterSmokeableCreator(memory,ST_ROCKET,0x1bb620);
    const auto hash=hp2::frontend::bStringHash("ROCKETEXPLD");
    if(!emitters)hp2::not_implemented("0x00127438","original munition explosion emitter-group lookup is not bound");
    emitters(memory,0x328160,hash);
}
// Debug001BC108, E-0099-W09.
void DroppedSpikes::InitDroppedSpikes(::port::driving::Memory& memory,const ::port::race::EmitterGroupLookup& emitters){
    memory.word(0x328158,bNewSlotPool(memory,0x490,8,0x2aea78,0x10));
    RegisterSmokeableCreator(memory,ST_DROPPED_SPIKES,0x1bb720);
    const auto hash=hp2::frontend::bStringHash("ROCKETEXPLD");
    if(!emitters)hp2::not_implemented("0x00127438","original munition explosion emitter-group lookup is not bound");
    emitters(memory,0x328170,hash);
}
// Debug001BB7A0, E-0099-W09. No projectile/physics construction is claimed.
void InitMunitions(::port::driving::Memory& memory,const ::port::race::EmitterGroupLookup& emitters){
    const auto handler=memory.allocate(8);memory.word(0x328188,handler);
    memory.word(handler,handler);memory.word(handler+4,handler);
    SmokeBomb::InitSmokeBomb(memory,emitters);
    Rocket::InitRocket(memory,emitters);
    DroppedSpikes::InitDroppedSpikes(memory,emitters);
}
// Debug001BBE08, E-0099-W12. Pool deletion precedes copied-emitter deletion.
void SmokeBomb::FiniSmokeBomb(::port::driving::Memory& memory){
    if(const auto pool=memory.word(0x328154)){bDeleteSlotPool(memory,pool);memory.word(0x328154,0);}
    afxDeleteEmitters(memory,0x328168);
}
// Debug001BC5B8, E-0099-W12.
void Rocket::FiniRocket(::port::driving::Memory& memory){
    if(const auto pool=memory.word(0x328150)){bDeleteSlotPool(memory,pool);memory.word(0x328150,0);}
    afxDeleteEmitters(memory,0x328160);
}
// Debug001BC160, E-0099-W12.
void DroppedSpikes::FiniDroppedSpikes(::port::driving::Memory& memory){
    if(const auto pool=memory.word(0x328158)){bDeleteSlotPool(memory,pool);memory.word(0x328158,0);}
    afxDeleteEmitters(memory,0x328170);
}
// Debug001BB7E8 /001BCA98, E-0099-W12. Empty handler is a real original branch,
// not suppression of active-effect removal. No handler mutation follows a gap.
void CloseMunitions(::port::driving::Memory& memory){
    SmokeBomb::FiniSmokeBomb(memory);Rocket::FiniRocket(memory);DroppedSpikes::FiniDroppedSpikes(memory);
    if(const auto handler=memory.word(0x328188)){
        if(memory.word(handler)!=handler)
            hp2::not_implemented("0x001BCA98","original nonempty MunitionEffectsHandler auto-emitter teardown is not implemented");
        memory.release(handler);
    }
    memory.word(0x328188,0);
}
}

namespace hp2::recovered {
// ELF 0x001BEF20; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001bef20.md.
void DroppedSpikes::Sub_001BEF20() {
    hp2::not_implemented("0x001BEF20", "DroppedSpikes::Sub_001BEF20 (signature and full body unresolved)");
}

// ELF 0x001BEF90; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001bef90.md.
void DroppedSpikes::DoSnapshot() {
    hp2::not_implemented("0x001BEF90", "DroppedSpikes::DoSnapshot (signature and full body unresolved)");
}

// ELF 0x001BF078; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001bf078.md.
void DroppedSpikes::MaybeExplodeWarhead() {
    hp2::not_implemented("0x001BF078", "DroppedSpikes::MaybeExplodeWarhead (signature and full body unresolved)");
}

// ELF 0x001BF0D8; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001bf0d8.md.
void DroppedSpikes::Move() {
    hp2::not_implemented("0x001BF0D8", "DroppedSpikes::Move (signature and full body unresolved)");
}

// ELF 0x001BF160; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001bf160.md.
void DroppedSpikes::ConsiderImpulse() {
    hp2::not_implemented("0x001BF160", "DroppedSpikes::ConsiderImpulse (signature and full body unresolved)");
}

// ELF 0x001BF178; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001bf178.md.
void DroppedSpikes::ApplyImpulse() {
    hp2::not_implemented("0x001BF178", "DroppedSpikes::ApplyImpulse (signature and full body unresolved)");
}

// ELF 0x001BFD90; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001bfd90.md.
void DroppedSpikes::GetSmokeType() {
    hp2::not_implemented("0x001BFD90", "DroppedSpikes::GetSmokeType (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001BEAD0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001bead0.md.
void Munition::Backup() {
    hp2::not_implemented("0x001BEAD0", "Munition::Backup (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001BF3D8; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001bf3d8.md.
void Rocket::Sub_001BF3D8() {
    hp2::not_implemented("0x001BF3D8", "Rocket::Sub_001BF3D8 (signature and full body unresolved)");
}

// ELF 0x001BF4D0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001bf4d0.md.
void Rocket::MaybeExplodeWarhead() {
    hp2::not_implemented("0x001BF4D0", "Rocket::MaybeExplodeWarhead (signature and full body unresolved)");
}

// ELF 0x001BF598; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001bf598.md.
void Rocket::Move() {
    hp2::not_implemented("0x001BF598", "Rocket::Move (signature and full body unresolved)");
}

// ELF 0x001BF8F8; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001bf8f8.md.
void Rocket::ConsiderImpulse() {
    hp2::not_implemented("0x001BF8F8", "Rocket::ConsiderImpulse (signature and full body unresolved)");
}

// ELF 0x001BF908; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001bf908.md.
void Rocket::ApplyImpulse() {
    hp2::not_implemented("0x001BF908", "Rocket::ApplyImpulse (signature and full body unresolved)");
}

// ELF 0x001BF910; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001bf910.md.
void Rocket::DoSnapshot() {
    hp2::not_implemented("0x001BF910", "Rocket::DoSnapshot (signature and full body unresolved)");
}

// ELF 0x001BFD10; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001bfd10.md.
void Rocket::GetSmokeType() {
    hp2::not_implemented("0x001BFD10", "Rocket::GetSmokeType (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001BEC28; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001bec28.md.
void SmokeBomb::Sub_001BEC28() {
    hp2::not_implemented("0x001BEC28", "SmokeBomb::Sub_001BEC28 (signature and full body unresolved)");
}

// ELF 0x001BED20; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001bed20.md.
void SmokeBomb::MaybeExplodeWarhead() {
    hp2::not_implemented("0x001BED20", "SmokeBomb::MaybeExplodeWarhead (signature and full body unresolved)");
}

// ELF 0x001BED78; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001bed78.md.
void SmokeBomb::Move() {
    hp2::not_implemented("0x001BED78", "SmokeBomb::Move (signature and full body unresolved)");
}

// ELF 0x001BFE10; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001bfe10.md.
void SmokeBomb::GetSmokeType() {
    hp2::not_implemented("0x001BFE10", "SmokeBomb::GetSmokeType (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/inc/Smokeable.hpp"
namespace hp2::debug {
// TASK-0099 E-0099-W79. Original concrete resource-family factories, no host callbacks.
std::uint32_t RocketSmokeableCreator(::port::driving::Memory& m,int id,std::uint32_t info,std::uint32_t hash){const auto body=bOMalloc(m,m.word(0x328150));Munition(m,body).Sub_001BB830(id,info,hash);m.word(body+0x10c,0x2aeab0);m.word(body+0x50,0x2aead0);Rocket(m,body).Init();return body;}
std::uint32_t SmokeBombSmokeableCreator(::port::driving::Memory& m,int id,std::uint32_t info,std::uint32_t hash){const auto body=bOMalloc(m,m.word(0x328154));DroppedBomb(m,body).Sub_001BBBF8(id,info,hash);m.word(body+0x10c,0x2aecb0);m.word(body+0x50,0x2aecd0);SmokeBomb(m,body).Init();return body;}
std::uint32_t DroppedSpikesSmokeableCreator(::port::driving::Memory& m,int id,std::uint32_t info,std::uint32_t hash){const auto body=bOMalloc(m,m.word(0x328158));DroppedBomb(m,body).Sub_001BBBF8(id,info,hash);m.word(body+0x10c,0x2aebb0);m.word(body+0x50,0x2aebd0);DroppedSpikes(m,body).Init();return body;}
// Debug001BB830/001BBBF8, exact SW BackupDone+470 (not a byte clear).
void Munition::Sub_001BB830(int id,std::uint32_t info,std::uint32_t hash){Smokeable(m_,address_).Sub_0021AE00(id,info,hash);m_.word(address_+0x10c,0x2aeeb0);m_.word(address_+0x50,0x2aeed0);m_.word(address_+0x470,0);}
void DroppedBomb::Sub_001BBBF8(int id,std::uint32_t info,std::uint32_t hash){Munition(m_,address_).Sub_001BB830(id,info,hash);m_.word(address_+0x10c,0x2aedb0);m_.word(address_+0x50,0x2aedd0);}
// Debug001BC558/001BBDA8: genuine JRRA/NOP bodies; no omitted sound/target init.
void Rocket::Init(){}
void SmokeBomb::Init(){}
// Debug001BC0A0: original JRRA delay SW clears full DeploySpikes word.
void DroppedSpikes::Init(){m_.word(address_+0x480,0);}
// Debug001BB878/001BBC38/001BC4F0/001BBD40/001BC038. Original nested
// flags0 teardown then exact static operator-delete pool on flags1 only.
void Munition::Sub_001BB878(int flags){m_.word(address_+0x10c,0x2aeeb0);m_.word(address_+0x50,0x2aeed0);Smokeable(m_,address_).Sub_0021B0D0(0);if(flags&1)SlotPool(m_,m_.word(0x328b2c)).Free(address_);}
void DroppedBomb::Sub_001BBC38(int flags){m_.word(address_+0x10c,0x2aedb0);m_.word(address_+0x50,0x2aedd0);Munition(m_,address_).Sub_001BB878(0);if(flags&1)SlotPool(m_,m_.word(0x328b2c)).Free(address_);}
void Rocket::Sub_001BC4F0(int flags){m_.word(address_+0x10c,0x2aeab0);m_.word(address_+0x50,0x2aead0);Munition(m_,address_).Sub_001BB878(0);if(flags&1)SlotPool(m_,m_.word(0x328150)).Free(address_);}
void SmokeBomb::Sub_001BBD40(int flags){m_.word(address_+0x10c,0x2aecb0);m_.word(address_+0x50,0x2aecd0);DroppedBomb(m_,address_).Sub_001BBC38(0);if(flags&1)SlotPool(m_,m_.word(0x328154)).Free(address_);}
void DroppedSpikes::Sub_001BC038(int flags){m_.word(address_+0x10c,0x2aebb0);m_.word(address_+0x50,0x2aebd0);DroppedBomb(m_,address_).Sub_001BBC38(0);if(flags&1)SlotPool(m_,m_.word(0x328158)).Free(address_);}
}

#include "game_core/indep/inc/Replay.hpp"
#include "game_core/indep/inc/World.hpp"
namespace hp2::debug {
// Debug001BCA28/001BC0A8, E-0099-W79. Original named sections/field/child order.
void Rocket::DoSnapshot(std::uint32_t snapshot,const ::port::race::SnapshotServices& services){ReplaySnapshot s(m_,snapshot);s.BeginSection(address_,0x328180);s.MoveData(address_+0x484,4);World(m_,m_.word(0x3272f0)).DoCarPointerSnapshot(address_+0x480,snapshot);Smokeable(m_,address_).DoSnapshot(snapshot,services);s.EndSection();}
void DroppedSpikes::DoSnapshot(std::uint32_t snapshot,const ::port::race::SnapshotServices& services){ReplaySnapshot s(m_,snapshot);s.BeginSection(address_,0x2aea68);s.MoveData(address_+0x480,1,4,0);Smokeable(m_,address_).DoSnapshot(snapshot,services);s.EndSection();}
}
