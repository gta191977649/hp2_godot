// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\world.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
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
#include "game_core/indep/inc/World.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x0014D338; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0014d338.md.
void World::BuildCarTable() {
    hp2::not_implemented("0x0014D338", "World::BuildCarTable (signature and full body unresolved)");
}

// ELF 0x0014D508; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0014d508.md.
void World::GetCarByNumber() {
    hp2::not_implemented("0x0014D508", "World::GetCarByNumber (signature and full body unresolved)");
}

// ELF 0x0014D638; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0014d638.md.
void World::ResetTimeScale() {
    hp2::not_implemented("0x0014D638", "World::ResetTimeScale (signature and full body unresolved)");
}

// ELF 0x0014E3E8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0014e3e8.md.
void World::Sub_0014E3E8() {
    hp2::not_implemented("0x0014E3E8", "World::Sub_0014E3E8 (signature and full body unresolved)");
}

// ELF 0x0014E440; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0014e440.md.
void World::Sub_0014E440() {
    hp2::not_implemented("0x0014E440", "World::Sub_0014E440 (signature and full body unresolved)");
}
} // namespace hp2::recovered
// TASK-0048 E-0048-T06. Debug0014D630, single active dynamic car against immutable track bodies.
#include "game_core/port/driving/static_collisions.hpp"
namespace hp2::recovered {
void World::DoTimestepCollisions(port::driving::Memory& m,port::driving::StaticCollisions& collisions,std::uint32_t body,std::uint32_t car,const port::track::Topology& topology,port::driving::CollisionMotion& motion,const std::function<void(float)>& move,const std::function<void()>& recompute){
    for(unsigned iteration=0;iteration<20;++iteration){const auto contacts=collisions.contacts(m,body,topology,motion);if(contacts.empty())return;collisions.resolve(m,body,car,contacts.front(),motion,move,recompute);}
}
}

namespace port::race {
// Port-only malformed-list guard. Floyd traversal requires no per-call allocations.
void validate_world_ring(::port::driving::Memory& memory,std::uint32_t head){
    const auto next=[&](std::uint32_t pointer){
        if(!pointer)hp2::not_implemented("0x0014D028","original World list contains null link");
        return memory.word(pointer);
    };
    auto slow=head,fast=head;
    do{
        slow=next(slow);fast=next(fast);if(fast!=head)fast=next(fast);
        if(fast==head)return;
        if(slow==fast)hp2::not_implemented("0x0014D028","original World list cycle does not reach sentinel");
    }while(true);
}
}
namespace hp2::debug {
// Debug0014D028, E-0099-W01. Original stable four traversals: player, AI racer,
// COP/SUPER_COP, traffic. Car.MyDriver+8 supplies type; unknown types are omitted.
void World::BuildCarTable(){
    auto& memory=memory_;const auto world=address_;
    ::port::race::validate_world_ring(memory,world);::port::race::validate_world_ring(memory,world+8);
    memory.word(world+0x1c,0);memory.word(world+0x20,0);memory.word(world+0x24,0);memory.word(world+0x28,0);
    unsigned count=0;
    const auto append_group=[&](int type,int alternative){
        for(auto car=memory.word(world);car!=world;car=memory.word(car)){
            const auto driver=memory.word(car+0x14);
            if(!driver)hp2::not_implemented("0x0014D028","original Car.MyDriver is not bound");
            const int actual=memory.signed_word(driver+8);
            if(actual!=type && actual!=alternative)continue;
            if(count>=14)hp2::not_implemented("0x0014D028","original World.CarTable exceeds fourteen entries");
            memory.word(world+0x2c+4*count,car);++count;memory.word(world+0x1c,count);
        }
    };
    append_group(1,1);memory.word(world+0x20,count);
    append_group(2,2);memory.word(world+0x24,count);
    append_group(4,5);memory.word(world+0x28,count);
    append_group(3,3);
    for(unsigned index=0;index<count;++index)memory.half(memory.word(world+0x2c+4*index)+8,static_cast<std::uint16_t>(index));
    // Original final AICarList traversal has no body; ring validity was checked above.
}
// Debug0014D1F8, E-0099-W01. Search signed Car.DriverNumber, not CarIndex or a table ordinal.
std::uint32_t World::GetCarByNumber(int number) const{
    const int count=memory_.signed_word(address_+0x1c);
    if(count<0 || count>14)hp2::not_implemented("0x0014D1F8","original World.CarTable count outside host bounds");
    for(int i=0;i<count;++i){const auto car=memory_.word(address_+0x2c+4*i);
        if(memory_.signed_half(car+0xa)==number)return car;}
    return 0;
}
// Debug0014D328, E-0099-W01. Preserve previous waste and all unrelated World fields.
void World::ResetTimeScale(){
    memory_.word(address_+0x190,0);memory_.scalar(address_+0x184,1);
    memory_.scalar(address_+0x188,0);memory_.scalar(address_+0x17c,0);memory_.word(address_+0x18c,0);
}
}
