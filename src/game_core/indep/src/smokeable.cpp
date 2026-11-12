// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\smokeable.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): Smokeable::Move() [0x002BCAD8]
#include "game_core/indep/inc/Smokeable.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/port/driving/ee_scalar.hpp"
#include "game_core/port/race/munitions.hpp"
#include <stdexcept>
#include <cmath>
#include "game_core/port/race/smokeables.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include "game_core/inc/bSlotPool.hpp"
#include <cstring>
#include "game_core/port/track/emitter_bindings.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/indep/inc/Scenery.hpp"

namespace port::race::detail {
// Port-only exception lifetime: an unresolved recovered destructor must not
// leave real body nodes referencing a released logical stack sentinel. Source
// has no C++ failure path; normal successful deletion order is unchanged.
struct SmokeablePendingDeletion {
 ::port::driving::Memory& m;std::uint32_t head,global;
 ~SmokeablePendingDeletion(){if(m.word(head)==head)return;const auto first=m.word(head),last=m.word(head+4),tail=m.word(global+4);m.word(tail,first);m.word(first+4,tail);m.word(last,global);m.word(global+4,last);m.word(head,head);m.word(head+4,head);}
};
std::uint32_t smokeable_pose(::port::driving::Memory&m,std::uint32_t body,unsigned slot,const char* call){const auto table=m.word(body+0x50),target=m.word(table+slot+4),receiver=body+static_cast<std::int32_t>(m.signed_half(table+slot));if(target==0x21c918)return hp2::debug::Smokeable(m,receiver).GetVelocity();if(target==0x21c928)return hp2::debug::Smokeable(m,receiver).GetPosition();hp2::not_implemented(call,"Original Smokeable pose virtual implementation is required");}
void smokeable_detach(::port::driving::Memory&m,std::uint32_t body){const auto next=m.word(body),prev=m.word(body+4);m.word(prev,next);m.word(next+4,prev);}
void smokeable_append(::port::driving::Memory&m,std::uint32_t head,std::uint32_t body){const auto tail=m.word(head+4);m.word(tail,body);m.word(head+4,body);m.word(body+4,tail);m.word(body,head);}
}

namespace hp2::debug {
// Debug00219890, E-0099-W09. Preserve creator identity exactly in the original
// table; no reinterpret_cast or invocation of PS2 addresses as native code.
void RegisterSmokeableCreator(::port::driving::Memory& memory,SmokeableType type,std::uint32_t creator){
    const auto index=static_cast<unsigned>(type);
    if(index>=ST_NUM_SMOKEABLE_TYPES)throw std::out_of_range("Original smokeable creator type outside table");
    memory.word(0x3138b0+4*index,creator);
}
// Debug0021C5C8, E-0099-W12. Process static initialization only.
void Sub_0021C5C8(::port::driving::Memory& memory){memory.word(0x328b44,0x328b40);memory.word(0x328b40,0x328b40);}
// Debug0021A4B0, E-0099-W12. First matching original intrusive-list node wins.
std::uint32_t FindSmokeableInfo(const ::port::driving::Memory& memory,std::uint32_t hash){
    for(auto info=memory.word(0x328b40);info!=0x328b40;info=memory.word(info))
        if(memory.word(info+0x70)==hash)return info;
    return 0;
}
// Debug0021A4E8, E-0099-W12. Preserve initialization/filter ordering. No caller
// may infer an empty original world from unbound/unparsed host scenery metadata.
void SpawnSmokeables(::port::driving::Memory& memory,const ::port::race::SmokeableResources& resources,
    const ::port::race::EmitterGroupLookup& emitters){
    if(!resources.source_data_loaded)hp2::not_implemented("0x0021A4E8","original smokeable/scenery source data has not been loaded");
    const auto count=memory.signed_word(0x328b38);if(count==0)return;
    for(auto info=memory.word(0x328b40);info!=0x328b40;info=memory.word(info)){
        if(!emitters)hp2::not_implemented("0x00127438","original smokeable emitter-group lookup is not bound");
        emitters(memory,info+0xd8,memory.word(info+0x74));
        emitters(memory,info+0xd0,memory.word(info+0x78));
    }
    memory.word(0x328b28,bNewSlotPool(memory,100,count,0x2b45b0,0x10));
    memory.word(0x328b2c,bNewSlotPool(memory,0x470,32,0x2b45c8,0x10));
    RegisterSmokeableCreator(memory,ST_BASIC_SMOKEABLE,0x2198e0);
    const auto spawners=memory.word(0x328b34);
    for(int i=0;i<memory.signed_word(0x328b38);++i){const auto spawner=spawners+8*std::uint32_t(i);
        const int section=memory.signed_half(spawner+4),instance=memory.signed_half(spawner+6);
        const auto found=resources.section_headers.find(section);if(found==resources.section_headers.end())continue;
        const auto header=found->second;
        if(instance<0 || instance>=memory.signed_word(header+0x20))throw std::out_of_range("Original smokeable spawner instance index invalid");
        const auto record=memory.word(header+0x1c)+std::uint32_t(instance)*0x30;
        if(memory.signed_word(0x311f78+0x30)==2 && (memory.half(record+0xe)&1))continue;
        if(!FindSmokeableInfo(memory,memory.word(spawner)))continue;
        const auto body=bOMalloc(memory,memory.word(0x328b28));
        if(!body)hp2::not_implemented("0x0021A9F8","original VirginSmokeable slot allocation failed");
        VirginSmokeable(memory,body).Sub_0021A9F8(spawner);
    }
}
// Debug0021A9F8, E-0099-W15. Real original static collision object and source
// scenery pointers; no single-car fixture or fabricated physics shape is used.
void VirginSmokeable::Sub_0021A9F8(std::uint32_t spawner){
    auto& m=memory_;const auto delta=std::int64_t(spawner)-m.word(0x328b34);
    if(delta<0 || delta%8)throw std::out_of_range("VirginSmokeable spawner is outside original table");
    Sub_001D5310(~int(delta/8),VIRGIN_SMOKEABLE_COLLISION_BODY);
    const auto count=m.word(0x31159c)+1;m.word(0x31159c,count);m.word(address_+0x50,0x2b4750);
    if(m.signed_word(0x3115c0)<std::int32_t(count))m.word(0x3115c0,count);
    m.word(address_+0x54,spawner);const auto section=m.word(0x2f0860+4*std::uint32_t(m.signed_half(spawner+4)));
    const auto info=FindSmokeableInfo(m,m.word(spawner));m.word(address_+0x58,info);
    const auto instances=m.word(section+0x1c);const int index=m.signed_half(spawner+6);
    if(index<0 || index>=m.signed_word(section+0x20))throw std::out_of_range("Original VirginSmokeable instance index invalid");
    const auto instance=instances+std::uint32_t(index)*0x30;m.word(address_+0x5c,instance);
    m.half(instance+0xe,m.half(instance+0xe)&0xffef);
    const int info_index=m.signed_half(instance+0xc);
    if(info_index<0 || info_index>=m.signed_word(section+0x18))throw std::out_of_range("Original VirginSmokeable scenery info index invalid");
    m.word(address_+0x60,m.word(section+0x14)+std::uint32_t(info_index)*0x28);
    std::string name="VirginSmokeable";
    // Original SmokeableInfo.Name is24 bytes and bSPrintf uses VirginSmokeable%s.
    for(std::uint32_t i=0;i<24;++i){const auto c=m.byte(info+8+i);if(!c)break;name.push_back(char(c));}
    SetDebugName(name);m.word(address_+8,0);m.byte(address_+0x11,0);m.byte(address_+0x13,0);
    const std::array<float,2> minimum{{float(m.signed_half(instance)),float(m.signed_half(instance+2))}};
    const std::array<float,2> maximum{{float(m.signed_half(instance+6)),float(m.signed_half(instance+8))}};
    SetBBoxMinMax(minimum,maximum);
    PlotToCaffeine(hp2::recovered::bIdentity(),{{m.scalar(instance+0x10),m.scalar(instance+0x14),m.scalar(instance+0x18)}});
}
// Debug0021ABB8, E-0099-W15. Restore the actual scenery instance bit and return
// the object to its original slot pool only after base/list/stat teardown.
void VirginSmokeable::Sub_0021ABB8(int in_chrg){
    auto& m=memory_;const auto instance=m.word(address_+0x5c);const auto flags=m.half(instance+0xe);
    m.word(0x31159c,m.word(0x31159c)-1);m.word(address_+0x50,0x2b4750);m.half(instance+0xe,flags|0x10);
    Sub_001D5370(0);if(in_chrg&1)SlotPool(m,m.word(0x328b28)).Free(address_);
}
// Debug0021A118, E-0099-W15. Detach matching types to a temporary list before
// dispatching destructors; this retains original sorted-list/max-width effects.
void DeleteSmokeableCollisionBodies(::port::driving::Memory& m){
    ::port::driving::Memory::Temporary storage(m,8);const auto head=storage.address();m.word(head,head);m.word(head+4,head);constexpr std::uint32_t bodies=0x3283d8;::port::race::detail::SmokeablePendingDeletion pending{m,head,bodies};
    for(auto body=m.word(bodies);body!=bodies;){const auto next=m.word(body);const int type=m.signed_byte(body+12);if(type==2||type==1){::port::race::detail::smokeable_detach(m,body);::port::race::detail::smokeable_append(m,head,body);}body=next;}
    // MIPS21A190..1C8 is reachable despite decompiler's unreachable warning:
    // every pending body is reinserted at global head BEFORE virtual destruction.
    while(m.word(head)!=head){const auto body=m.word(head),table=m.word(body+0x50),target=m.word(table+12);::port::race::detail::smokeable_detach(m,body);const auto first=m.word(bodies);m.word(bodies,body);m.word(first+4,body);m.word(body,first);m.word(body+4,bodies);const auto receiver=body+static_cast<std::int32_t>(m.signed_half(table+8));
        if(target==0x21abb8)VirginSmokeable(m,receiver).Sub_0021ABB8(3);else if(target==0x21b0d0)Smokeable(m,receiver).Sub_0021B0D0(3);else if(target==0x1bc4f0)Rocket(m,receiver).Sub_001BC4F0(3);else if(target==0x1bbd40)SmokeBomb(m,receiver).Sub_001BBD40(3);else if(target==0x1bc038)DroppedSpikes(m,receiver).Sub_001BC038(3);else hp2::not_implemented("0x0021A1C4","Original selected Smokeable-family virtual destructor implementation is required");
    }
}

// Debug0021A658, E-0099-W15. Actual construction-only resources are freed.
void ClearSmokeables(::port::driving::Memory& m){
    DeleteSmokeableCollisionBodies(m);
    if(const auto pool=m.word(0x328b28)){bDeleteSlotPool(m,pool);m.word(0x328b28,0);}
    if(const auto pool=m.word(0x328b2c)){bDeleteSlotPool(m,pool);m.word(0x328b2c,0);}
    for(auto info=m.word(0x328b40);info!=0x328b40;info=m.word(info)){
        afxDeleteEmitters(m,info+0xd8);afxDeleteEmitters(m,info+0xd0);
    }
    m.word(0x328b2c,0);
}
}

namespace port::race {
// Port-only validated copy of Debug LoaderSmokeables00219C90 /MemoryImageLoad
// 00219C48 and independently reviewed scenery record spans. E-0099-W12.
SmokeableResources load_smokeable_resources(::port::driving::Memory& memory,
    const std::vector<std::uint8_t>& global,const std::vector<std::uint8_t>& track,bool reverse){
    SmokeableResources out;
    if(memory.word(0x328b40)!=0x328b40 || memory.word(0x328b44)!=0x328b40 || memory.word(0x328b38)!=0)
        throw std::logic_error("Smokeable source adapter requires a fresh initialized runtime");
    const auto load=[&](const std::vector<std::uint8_t>& bytes){
        const auto word=[&](std::size_t at){if(at>bytes.size()||bytes.size()-at<4)throw std::out_of_range("Truncated smokeable source word");
            return std::uint32_t(bytes[at])|(std::uint32_t(bytes[at+1])<<8)|(std::uint32_t(bytes[at+2])<<16)|(std::uint32_t(bytes[at+3])<<24);};
        const auto half=[&](std::size_t at){if(at>bytes.size()||bytes.size()-at<2)throw std::out_of_range("Truncated smokeable source half");
            return std::uint16_t(bytes[at]|(std::uint16_t(bytes[at+1])<<8));};
        const auto copy=[&](std::size_t at,std::size_t size){const auto address=memory.allocate(std::uint32_t(size));
            std::memcpy(memory.view(address,size).data,bytes.data()+at,size);out.owned_allocations.push_back(address);return address;};
        const auto payload=[&](const hp2::assets::Chunk& chunk,std::size_t begin,std::size_t size){
            return size?copy(begin,size):copy(chunk.header_offset,8)+8;};
        const auto chunks=hp2::assets::parse_chunks_top(bytes);
        std::function<void(const std::vector<hp2::assets::Chunk>&)> visit;
        visit=[&](const auto& siblings){for(const auto& chunk:siblings){
            if(chunk.tag==0x34026){const auto begin=(chunk.header_offset+0x17)&~std::size_t(15),end=chunk.payload_offset()+chunk.size;
                if(begin>end || (end-begin)%0xe0)throw std::out_of_range("Invalid original SmokeableInfo record span");
                for(auto at=begin;at<end;at+=0xe0){const auto info=copy(at,0xe0);out.info_records.push_back(info);const auto previous=memory.word(0x328b44);
                    memory.word(previous,info);memory.word(info+4,previous);memory.word(0x328b44,info);memory.word(info,0x328b40);
                    const auto terrain=hp2::debug::MatchTerrainType(memory,memory.word(info+0xc4));
                    memory.word(info+0xd8,info+0xd8);memory.word(info+0xdc,info+0xd8);memory.word(info+0xd0,info+0xd0);
                    memory.word(info+0xc0,terrain);memory.word(info+0xd4,info+0xd0);
                }
                for(auto info=memory.word(0x328b40);info!=0x328b40;info=memory.word(info))for(int child=0;child<4;++child){
                    const auto hash=memory.word(info+0x90+4*child);if(hash)memory.word(info+0x80+4*child,hp2::debug::FindSmokeableInfo(memory,hash));}
            }
            if(chunk.tag==0x34027){if(chunk.size%8)throw std::out_of_range("Invalid original SmokeableSpawner record span");
                // Even an empty original chunk assigns its non-null payload
                // address. Keep an actual copied header and use its end pointer.
                const auto data=chunk.size?copy(chunk.payload_offset(),chunk.size):copy(chunk.header_offset,8)+8;
                memory.word(0x328b34,data);memory.word(0x328b38,chunk.size>>3);out.spawner_data_loaded=true;}
            if(chunk.tag==0x80034100){int section=-1;std::uint32_t header=0;
                const hp2::assets::Chunk *instances=nullptr,*infos=nullptr,*tree=nullptr;
                for(const auto& child:chunk.children){if(child.tag==0x34101){const auto at=(child.header_offset+0x17)&~std::size_t(15);
                        if(at+56>child.payload_offset()+child.size)throw std::out_of_range("Truncated smokeable scenery section header");
                        section=int(word(at+8));header=copy(at,56);}
                    if(child.tag==0x34102)infos=&child;
                    if(child.tag==0x34103)instances=&child;
                    if(child.tag==0x34104)tree=&child;}
                if(section<0 || !header || !instances || !infos)throw std::out_of_range("Incomplete original smokeable scenery records");
                if(out.section_exclude_flags.count(section))throw std::out_of_range("Duplicate original smokeable scenery section");
                const auto begin=(instances->header_offset+0x17)&~std::size_t(15),end=instances->payload_offset()+instances->size;
                if(begin>end || (end-begin)%48)throw std::out_of_range("Invalid original smokeable scenery instance span");
                auto& flags=out.section_exclude_flags[section];for(auto at=begin;at<end;at+=48)flags.push_back(half(at+14));
                if(infos->size%0x28)throw std::out_of_range("Invalid original scenery info record span");
                // LoaderScenery0014E920 uses the unaligned Info payload, the
                // aligned Instance span, and copies no replacement collision
                // geometry. Retain every byte of both actual record arrays.
                memory.word(header+0x14,payload(*infos,infos->payload_offset(),infos->size));
                memory.word(header+0x18,infos->size/0x28);
                memory.word(header+0x1c,payload(*instances,begin,end-begin));memory.word(header+0x20,(end-begin)/0x30);
                if(tree){if(tree->size%0x18)throw std::out_of_range("Invalid original scenery tree record span");
                    memory.word(header+0x24,payload(*tree,tree->payload_offset(),tree->size));memory.word(header+0x28,tree->size/0x18);}
                // This port adapter publishes only the constructor-consumed
                // section table. Scenery render/model loading is separate.
                out.section_headers.emplace(section,header);out.section_load_order.push_back(header);memory.word(0x2f0860+4*std::uint32_t(section),header);
                if(reverse)hp2::recovered::ScenerySectionHeader::FlipObjectsOnBackwardsTrack(memory,header);
            }
            visit(chunk.children);
        }};visit(chunks);
    };
    load(global);load(track);out.source_data_loaded=true;return out;
}
void unload_smokeable_resources(::port::driving::Memory& m,SmokeableResources& resources){
    if(!resources.source_data_loaded)throw std::logic_error("Smokeable source owner is not loaded");
    if(resources.scenery_models_initialized)throw std::logic_error("Close scenery models before raw scenery ownership teardown");
    if(m.word(0x328b28) || m.word(0x328b2c))throw std::logic_error("ClearSmokeables must finish before source ownership teardown");
    for(const auto info:resources.info_records)for(const auto offset:{0xd0u,0xd8u})
        if(m.word(info+offset)!=info+offset || m.word(info+offset+4)!=info+offset)
            throw std::logic_error("Smokeable source emitter list remains live during teardown");
    for(const auto& entry:resources.section_headers)
        if(m.word(0x2f0860+4*std::uint32_t(entry.first))!=entry.second)
            throw std::logic_error("Smokeable source scenery section ownership changed before teardown");
    // UnloaderSmokeables00219DE0: detach exactly the loaded Info records, then
    // reset the original34027 globals. Scenery render objects were never owned.
    for(const auto info:resources.info_records){const auto next=m.word(info),previous=m.word(info+4);m.word(previous,next);m.word(next+4,previous);}
    if(resources.spawner_data_loaded){m.word(0x328b34,0);m.word(0x328b38,0);}
    for(const auto& entry:resources.section_headers)m.word(0x2f0860+4*std::uint32_t(entry.first),0);
    for(auto allocation=resources.owned_allocations.rbegin();allocation!=resources.owned_allocations.rend();++allocation)m.release(*allocation);
    resources=SmokeableResources{};
}
}

namespace hp2::recovered {
// ELF 0x00220690; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00220690.md.
void Smokeable::Sub_00220690() {
    hp2::not_implemented("0x00220690", "Smokeable::Sub_00220690 (signature and full body unresolved)");
}

// ELF 0x002208A0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002208a0.md.
void Smokeable::DoSnapshot() {
    hp2::not_implemented("0x002208A0", "Smokeable::DoSnapshot (signature and full body unresolved)");
}

// ELF 0x00220CE8; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00220ce8.md.
void Smokeable::Move() {
    hp2::not_implemented("0x00220CE8", "Smokeable::Move (signature and full body unresolved)");
}

// ELF 0x002212D0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002212d0.md.
void Smokeable::MaybeExplodeWarhead() {
    hp2::not_implemented("0x002212D0", "Smokeable::MaybeExplodeWarhead (signature and full body unresolved)");
}

// ELF 0x00221340; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00221340.md.
void Smokeable::ConsiderImpulse() {
    hp2::not_implemented("0x00221340", "Smokeable::ConsiderImpulse (signature and full body unresolved)");
}

// ELF 0x00221F00; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00221f00.md.
void Smokeable::GetFriction() {
    hp2::not_implemented("0x00221F00", "Smokeable::GetFriction (signature and full body unresolved)");
}

// ELF 0x00221F10; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00221f10.md.
void Smokeable::GetBounciness() {
    hp2::not_implemented("0x00221F10", "Smokeable::GetBounciness (signature and full body unresolved)");
}

// ELF 0x00221F20; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00221f20.md.
void Smokeable::GetTerrainType() {
    hp2::not_implemented("0x00221F20", "Smokeable::GetTerrainType (signature and full body unresolved)");
}

// ELF 0x00221F30; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00221f30.md.
void Smokeable::IsMoveable() {
    hp2::not_implemented("0x00221F30", "Smokeable::IsMoveable (signature and full body unresolved)");
}

// ELF 0x00221F58; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00221f58.md.
void Smokeable::GetInertiaTensorInverse() {
    hp2::not_implemented("0x00221F58", "Smokeable::GetInertiaTensorInverse (signature and full body unresolved)");
}

// ELF 0x00221F88; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00221f88.md.
void Smokeable::GetMassInverse() {
    hp2::not_implemented("0x00221F88", "Smokeable::GetMassInverse (signature and full body unresolved)");
}

// ELF 0x00221FB8; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00221fb8.md.
void Smokeable::AreCollisionsEnabled() {
    hp2::not_implemented("0x00221FB8", "Smokeable::AreCollisionsEnabled (signature and full body unresolved)");
}

// ELF 0x00221FD8; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00221fd8.md.
void Smokeable::IsBlinking() {
    hp2::not_implemented("0x00221FD8", "Smokeable::IsBlinking (signature and full body unresolved)");
}

// ELF 0x00221FF0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00221ff0.md.
void Smokeable::ApplyImpulse() {
    hp2::not_implemented("0x00221FF0", "Smokeable::ApplyImpulse (signature and full body unresolved)");
}

// ELF 0x00222010; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00222010.md.
void Smokeable::GetRotation() {
    hp2::not_implemented("0x00222010", "Smokeable::GetRotation (signature and full body unresolved)");
}

// ELF 0x00222020; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00222020.md.
void Smokeable::GetVelocity() {
    hp2::not_implemented("0x00222020", "Smokeable::GetVelocity (signature and full body unresolved)");
}

// ELF 0x00222030; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00222030.md.
void Smokeable::GetPosition() {
    hp2::not_implemented("0x00222030", "Smokeable::GetPosition (signature and full body unresolved)");
}

// ELF 0x00222050; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00222050.md.
void Smokeable::GetSmokeType() {
    hp2::not_implemented("0x00222050", "Smokeable::GetSmokeType (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/inc/World.hpp"
namespace hp2::debug {
// E-0099-W116: original inline GetVelocity/GetPosition bodies moved unchanged
// to their freshly corroborated indep/inc/Smokeable.hpp definition owner.
// Debug0021A1F8, E-0099-W57. Original unsigned periodic counter gate;
// no synthetic tick increment or production disable flag.
void SmokeableThink(::port::driving::Memory& m){if(((m.word(0x327d10)+0x4du)&0x7fu)==0)DeleteFarAwaySmokeableCollisionBodies(m);}
}


namespace hp2::debug {
// Debug0021A220, E-0099-W57. Real two-pass source filtering, temporary deletion
// ring and reinsertion before virtual destruction. Unknown live destructors are
// required at the original call, never replaced with manual unlink/slot release.
void DeleteFarAwaySmokeableCollisionBodies(::port::driving::Memory& m){
 const auto pool=m.word(0x328b2c);if(!pool)return;::port::driving::Memory::Temporary storage(m,8);const auto head=storage.address();m.word(head,head);m.word(head+4,head);constexpr std::uint32_t bodies=0x3283d8;::port::race::detail::SmokeablePendingDeletion pending{m,head,bodies};
 for(auto body=m.word(bodies);body!=bodies;){const auto next=m.word(body);if(m.signed_byte(body+12)==1){const auto velocity=::port::race::detail::smokeable_pose(m,body,0x20,"0x0021A28C");const float x=m.scalar(velocity)*m.scalar(velocity),y=m.scalar(velocity+4)*m.scalar(velocity+4),z=m.scalar(velocity+8)*m.scalar(velocity+8);if(1000<std::sqrt((x+y)+z)&&!m.word(m.word(body+0x110)+0xcc)){::port::race::detail::smokeable_detach(m,body);::port::race::detail::smokeable_append(m,head,body);}}body=next;}
 if(bCountFreeSlots(m,pool)<10){for(auto body=m.word(bodies);body!=bodies;){const auto next=m.word(body);if(m.signed_byte(body+12)==1){const auto position=::port::race::detail::smokeable_pose(m,body,0x18,"0x0021A35C"),world=m.word(0x3272f0);if(!world)hp2::not_implemented("0x0014E128","Original current World cached player positions are required");const float distance=World(m,world).GetDistanceToClosestPlayer(std::array<float,3>{{m.scalar(position),m.scalar(position+4),m.scalar(position+8)}});if(!m.byte(body+0x462)){const auto awoken=m.word(body+0x120);float age=0;if(awoken!=0&&awoken!=0x7fffffff)age=static_cast<float>(static_cast<std::int32_t>(m.word(0x327cf8)-awoken))*m.scalar(0x325f74);if(45<age&&120<distance&&!m.word(m.word(body+0x110)+0xcc)){::port::race::detail::smokeable_detach(m,body);::port::race::detail::smokeable_append(m,head,body);}}}body=next;}}
 while(m.word(head)!=head){const auto body=m.word(head),table=m.word(body+0x50),target=m.word(table+12);::port::race::detail::smokeable_detach(m,body);const auto first=m.word(bodies);m.word(bodies,body);m.word(first+4,body);m.word(body,first);m.word(body+4,bodies);const auto receiver=body+static_cast<std::int32_t>(m.signed_half(table+8));
  if(target==0x21b0d0)Smokeable(m,receiver).Sub_0021B0D0(3);else if(target==0x1bc4f0)Rocket(m,receiver).Sub_001BC4F0(3);else if(target==0x1bbd40)SmokeBomb(m,receiver).Sub_001BBD40(3);else if(target==0x1bc038)DroppedSpikes(m,receiver).Sub_001BC038(3);else hp2::not_implemented("0x0021A45C","Original selected Smokeable-family virtual destructor implementation is required");
 }
}
}

#include "game_core/indep/inc/AcidFX.hpp"
#include "game_core/indep/inc/WorldObject.hpp"
#include "game_core/indep/inc/ComponentPhysics.hpp"
#include "game_core/indep/inc/Polyhedra.hpp"
#include "game_core/indep/src/voronoiclip.hpp"
#include "game_core/indep/src/track.hpp"
#include "game_core/src/bList.hpp"
namespace hp2::debug {
// Debug0021B0D0, E-0099-W57. Original derived-vtables/statistic, borrowed-vs-owned
// poly, emitter PLists, model/topology/components/base-body order. Existing
// Caffeine/unknown physics component callees remain their explicit boundaries.
void Smokeable::Sub_0021B0D0(int flags){auto& m=memory_;const auto self=address_;m.word(self+0x10c,0x2b4650);const auto count=m.word(0x3115a0);m.word(self+0x50,0x2b4670);m.word(0x3115a0,count-1);
 if(!FindPipelinePolyhedron(m,m.word(m.word(self+0x110)+0x70),nullptr))if(const auto poly=m.word(self+8))hp2::recovered::VCPolyhedron::Sub_001D00E8(m,poly,3);
 for(unsigned group=0;group<2;++group){const auto head=self+0x450+group*8;while(m.word(head)!=head){const auto node=m.word(head),emitter=m.word(node+8),next=m.word(node),prev=m.word(node+4);m.word(prev,next);m.word(next+4,prev);bPNode::Free(m,node);
   if(m.byte(self+0x460+group))AcidEffectSystem(m,0x2e6f40).RemoveAutoEmitter(emitter);if(emitter)AcidEmitter(m,emitter).Sub_00124FA0(3);
  }}
 WorldObject(m,self+0x3f0).Sub_001CA7F8(2);TopologyCoordinate(m,self+0x12c).Sub_001518C0(2);hp2::recovered::PhysicsSystem(m,self+0x100).Sub_00199AB8(0);DynamicCollisionBody(m,self).Sub_001D3C60(0);if(flags&1)SlotPool(m,m.word(0x328b2c)).Free(self);
}
}

#include "game_core/indep/inc/Replay.hpp"
#include "game_core/indep/src/rigidbody.hpp"
namespace hp2::debug {
// Debug00219C28/00219C08, E-0099-W76. Source null/-1 and arithmetic eight-byte table index.
int GetSmokeableSpawnerNumber(const ::port::driving::Memory& m,std::uint32_t p){return p?(std::int32_t(p-m.word(0x328b34))>>3):-1;}
std::uint32_t GetSmokeableSpawner(const ::port::driving::Memory& m,int n){return n<0?0:m.word(0x328b34)+8*std::uint32_t(n);}
// Debug0021C948, E-0099-W76; exact JR RA/MOVE V0,ZERO source getter.
// Debug0021AC40, E-0099-W76. Named source section over the actual static body path.
void VirginSmokeable::DoSnapshot(std::uint32_t snapshot){ReplaySnapshot s(memory_,snapshot);s.BeginSection(0x2b4608);StaticCollisionBody::DoSnapshot(snapshot);s.EndSection();}
// Debug0021B2E0, E-0099-W76. Source field order, spawner-number binding and
// actual take/restore RigidBody/cache effects, with explicit CPU policy context.
void Smokeable::DoSnapshot(std::uint32_t snapshot,const ::port::race::SnapshotServices& services){auto& m=memory_;ReplaySnapshot s(m,snapshot);s.BeginSection(address_,0x2b4628);DynamicCollisionBody::DoSnapshot(snapshot);s.MoveData(address_+0x462,1,1,1);s.MoveData(address_+0x463,1,1,1);s.VerifyData(m.word(address_+0x110)+0x70,4,4,0);s.MoveData(address_+0x114,4);s.MoveData(address_+0x118,4);for(auto offset:{0x120u,0x11cu,0x124u})s.MoveData(address_+offset,4);::port::driving::Memory::Temporary temp(m,2);const auto p=temp.address();if(!m.byte(snapshot+0x3d))m.half(p,std::uint16_t(GetSmokeableSpawnerNumber(m,m.word(address_+0x128))));s.MoveData(p,2,2,1);if(m.byte(snapshot+0x3d))m.word(address_+0x128,GetSmokeableSpawner(m,m.signed_half(p)));hp2::recovered::RigidBody(m,m.word(address_+0x60)).DoSnapshot(snapshot,services);if(m.byte(snapshot+0x3c)||m.byte(snapshot+0x3d))UpdatePosition();s.VerifyData(address_+0x14,8);s.VerifyData(address_+0x1c,8);s.EndSection();}
// Debug0021A6E0, E-0099-W76. Original two-phase actual body-list stream and
// FFFF delimiters. Active derived family/factory targets remain explicit.
void SnapshotAllSmokeables(::port::driving::Memory& m,std::uint32_t snapshot,const ::port::race::SnapshotServices& services){ReplaySnapshot s(m,snapshot);s.BeginSection(0x2b45e0);s.MoveData(0x328b30,4,4,1);::port::driving::Memory::Temporary temp(m,16);const auto p=temp.address();
 const auto virgin=[&](std::uint32_t body){const auto table=m.word(body+0x50),target=m.word(table+0x14),self=body+std::int32_t(m.signed_half(table+0x10));if(target!=0x21ac40)hp2::not_implemented("0x0021A6E0","unknown original VirginSmokeable.DoSnapshot virtual target");VirginSmokeable(m,self).DoSnapshot(snapshot);};
 const auto smoke=[&](std::uint32_t body){const auto table=m.word(body+0x50),target=m.word(table+0x14),self=body+std::int32_t(m.signed_half(table+0x10));if(target==0x21b2e0)Smokeable(m,self).DoSnapshot(snapshot,services);else if(target==0x1bca28)Rocket(m,self).DoSnapshot(snapshot,services);else if(target==0x1bc0a8)DroppedSpikes(m,self).DoSnapshot(snapshot,services);else hp2::not_implemented("0x0021A6E0","unknown original active Smokeable.DoSnapshot virtual target");};
 if(!m.byte(snapshot+0x3d)){
  for(auto body=m.word(0x3283d8);body!=0x3283d8;body=m.word(body))if(m.signed_byte(body+0xc)==2){m.half(p,std::uint16_t(GetSmokeableSpawnerNumber(m,m.word(body+0x54))));s.MoveData(p,2,2,1);virgin(body);}m.half(p,0xffff);s.MoveData(p,2,2,1);
  for(auto body=m.word(0x3283d8);body!=0x3283d8;body=m.word(body))if(m.signed_byte(body+0xc)==1){const auto info=m.word(body+0x110);m.half(p,m.half(body+0xe));m.word(p+4,m.word(info+0x70));m.word(p+8,WorldObject(m,body+0x3f0).GetNameHash());const auto table=m.word(body+0x50),target=m.word(table+0xd4),self=body+std::int32_t(m.signed_half(table+0xd0));int type;if(target==0x21c948)type=Smokeable(m,self).GetSmokeType();else if(target==0x1bce28)type=Rocket(m,self).GetSmokeType();else if(target==0x1bcf28)type=SmokeBomb(m,self).GetSmokeType();else if(target==0x1bcea8)type=DroppedSpikes(m,self).GetSmokeType();else hp2::not_implemented("0x0021A6E0","original derived GetSmokeType snapshot target is not reconstructed");m.byte(p+12,std::uint8_t(type));s.MoveData(p,2,2,0);s.MoveData(p+4,4,4,0);s.MoveData(p+8,4,4,0);s.MoveData(p+12,1);smoke(body);}m.half(p,0xffff);s.MoveData(p,2,2,1);
 }else{
  DeleteSmokeableCollisionBodies(m);for(;;){s.MoveData(p,2,2,0);if(m.half(p)==0xffff)break;const auto spawner=GetSmokeableSpawner(m,m.half(p)),body=bOMalloc(m,m.word(0x328b28));VirginSmokeable object(m,body);object.Sub_0021A9F8(spawner);virgin(body);}
  for(;;){s.MoveData(p+2,2,2,0);if(m.half(p+2)==0xffff)break;s.MoveData(p+4,4,4,0);s.MoveData(p+8,4,4,0);s.MoveData(p+12,1);const auto body=CreateSmokeable(m,SmokeableType(m.byte(p+12)),int(m.half(p+2)),FindSmokeableInfo(m,m.word(p+4)),m.word(p+8));smoke(body);}
 }
 s.EndSection();
}
}

namespace hp2::debug {
// Debug002198A8/002198E0, E-0099-W79. Concrete original creator identities only.
std::uint32_t CreateSmokeable(::port::driving::Memory& m,SmokeableType type,int id,std::uint32_t info,std::uint32_t hash){const auto index=std::uint32_t(type);if(index>=4)hp2::not_implemented("0x002198A8","Smokeable creator index outside recovered four-entry domain");switch(m.word(0x3138b0+4*index)){case 0x2198e0:return BasicSmokeableCreator(m,id,info,hash);case 0x1bb620:return RocketSmokeableCreator(m,id,info,hash);case 0x1bb6a0:return SmokeBombSmokeableCreator(m,id,info,hash);case 0x1bb720:return DroppedSpikesSmokeableCreator(m,id,info,hash);default:hp2::not_implemented("0x002198CC","Original registered Smokeable creator target is required");}}
std::uint32_t BasicSmokeableCreator(::port::driving::Memory& m,int id,std::uint32_t info,std::uint32_t hash){const auto body=bOMalloc(m,m.word(0x328b2c));Smokeable(m,body).Sub_0021AE00(id,info,hash);return body;}
// Debug0021AE00, E-0099-W79. Source order, real resource ownership and pools.
void Smokeable::Sub_0021AE00(int id,std::uint32_t info,std::uint32_t hash){auto& m=memory_;const auto a=address_;
 DynamicCollisionBody::Sub_001D3BE0(id,SMOKEABLE_COLLISION_BODY,a+0x12c);hp2::recovered::PhysicsSystem system(m,a+0x100);system.port_initialize();
 for(auto off:{0x11cu,0x120u,0x124u})m.word(a+off,0);m.word(a+0x10c,0x2b4650);m.word(a+0x50,0x2b4670);TopologyCoordinate(m,a+0x12c).Sub_00151878();WorldObject(m,a+0x3f0).Sub_001CA730(hash,0,false);
 for(auto head:{a+0x450,a+0x458}){m.word(head,head);m.word(head+4,head);}const auto count=m.word(0x3115a0)+1;m.word(0x3115a0,count);if(m.signed_word(0x3115c4)<std::int32_t(count))m.word(0x3115c4,count);
 const auto body=bOMalloc(m,m.word(0x327020));hp2::recovered::RigidBody(m,body).port_initialize(info+0x20);m.byte(a+0x11,0);m.word(a+0x54,1);m.byte(a+0x13,0);m.word(a+0x5c,a+0x100);m.word(a+0x120,0);m.word(a+0x124,0);m.word(a+0x60,body);m.word(a+0x11c,m.word(0x327cf8));
 std::string name="Smokeable";for(unsigned i=0;;++i){if(i>=39)hp2::not_implemented("0x0024A7A0","Smokeable debug name exceeds original48-byte formatted stack domain");const auto c=m.byte(info+8+i);if(!c)break;name.push_back(char(c));}SetDebugName(name);
 const std::array<float,3> cog{{m.scalar(info+0xa0),m.scalar(info+0xa4),m.scalar(info+0xa8)}};auto poly=FindPipelinePolyhedron(m,m.word(info+0x70),&cog);if(!poly){poly=m.allocate(0xaf0);hp2::recovered::RectPolyhedron::Sub_00212EB0(m,poly,1.5f,-1.5f,1.5f,-1.5f,1.5f,-1.5f);}m.word(a+8,poly);BuildBoundingVolumes();system.AddComponent(body);
 m.byte(a+0x462,0);m.byte(a+0x463,0);m.word(a+0x110,info);m.word(a+0x128,0);m.word(a+0x114,m.word(info+0xb4));m.word(a+0x118,m.word(info+0xb8));if(!DoesChildArtExist())m.word(a+0x118,0);Calibrate();m.byte(a+0x460,0);m.byte(a+0x461,0);
 for(unsigned group=0;group<2;++group){const auto source_offset=group?0xd0u:0xd8u,hash_offset=group?0x78u:0x74u,target=a+(group?0x458:0x450);if(!m.word(info+hash_offset))continue;for(auto source=m.word(m.word(a+0x110)+source_offset);source!=m.word(a+0x110)+source_offset;source=m.word(source)){const auto emitter=afxCopyEmitter(m,source);if(!emitter)continue;m.word(emitter+0x10,1);const auto node=bPNode::Malloc(m),first=m.word(target);m.word(target,node);m.word(first+4,node);m.word(node,first);m.word(node+8,emitter);m.word(node+4,target);}}
}
void Smokeable::Calibrate(){hp2::recovered::RigidBody body(memory_,memory_.word(address_+0x60));body.Calibrate(0);body.CalculateInertiaTensor();}
// Debug0021C170. Original8-byte postfix copied first; seeded0024C8F0 hash inlined.
int Smokeable::DoesChildArtExist(){auto& m=memory_;const int count=m.signed_word(m.word(address_+0x110)+0x7c);std::array<std::uint8_t,8> text;for(unsigned j=0;j<8;++j)text[j]=m.byte(0x328b50+j);for(int i=0;i<count;++i){text[6]=std::uint8_t(std::uint32_t(i)+0x31u);auto hash=WorldObject(m,address_+0x3f0).GetNameHash();for(auto c:text){if(!c)break;hash=hash*33u+c;}if(!WorldObject::DoesArtExist(m,hash))return 0;}return 1;}
}

#include "game_core/port/race/physics_construction.hpp"
#include "game_core/src/bDebug.hpp"
namespace hp2::debug {
// Debug0021AC90, E-0099-W83. Actual source pool-full predicate002510E0,
// factory identity, scenery rotation and spawner remain concrete.
std::uint32_t VirginSmokeable::ConvertToSmokeable(const ::port::race::PhysicsConstructionServices& service){
 auto& m=memory_;const auto pool=m.word(0x328b2c);
 if(!pool)hp2::not_implemented("0x002510E0","original Smokeable pool is not initialized");
 if(bIsSlotPoolFull(m,pool))return 0;
 const auto instance=m.word(address_+0x5c);
 ::port::driving::Memory::Temporary local(m,64);const auto matrix=local.address();
 for(unsigned column=0;column<3;++column){
  for(unsigned row=0;row<3;++row)m.scalar(matrix+16*column+4*row,float(m.signed_half(instance+0x1c+6*column+2*row))*0.00006103515625f);
  m.word(matrix+16*column+12,0);
 }
 for(unsigned row=0;row<3;++row)m.word(matrix+0x30+4*row,m.word(instance+0x10+4*row));
 m.scalar(matrix+0x3c,1);
 const auto id=GetNextCollisionBodyID(m);
 const auto body=CreateSmokeable(m,ST_BASIC_SMOKEABLE,id,m.word(address_+0x58),m.word(m.word(address_+0x60)));
 m.word(body+0x128,m.word(address_+0x54));
 Smokeable(m,body).SetInitialPosition(matrix,0,service);
 return body;
}
// Debug0021A9A8, E-0099-W83. The old Virgin is destroyed even when the
// member returns null for a full target pool; this is not a transactional retry.
std::uint32_t ConvertToSmokeable(::port::driving::Memory& m,std::uint32_t body,const ::port::race::PhysicsConstructionServices& service){
 const auto result=VirginSmokeable(m,body).ConvertToSmokeable(service);
 if(body){const auto table=m.word(body+0x50),target=m.word(table+0xc),self=body+std::int32_t(m.signed_half(table+8));
  if(target!=0x21abb8)hp2::not_implemented("0x0021A9D8","original Virgin conversion destructor target is not reconstructed");
  VirginSmokeable(m,self).Sub_0021ABB8(3);
 }
 return result;
}
// Debug0021BF98, E-0099-W83. No invented already-awake early return: source
// increments the counter on every call; emitter registration has its own gate.
void Smokeable::Awaken(){auto& m=memory_;const auto a=address_;
 m.byte(a+0x13,1);m.byte(a+0x462,1);m.word(0x328b30,m.word(0x328b30)+1);
 if(!m.byte(a+0x461)){const auto head=a+0x458;for(auto node=m.word(head);node!=head;node=m.word(node))AcidEffectSystem(m,0x2e6f40).AddAutoEmitter(m.word(node+8));m.byte(a+0x461,1);}
 if(!m.word(a+0x120)||m.word(a+0x120)==0x7fffffff)m.word(a+0x120,m.word(0x327cf8));
}
// Debug0021C210, E-0099-W83. Source XYZ lengths, signed wrapped Timer
// difference and strict two-second test. Fourth lanes do not enter the lengths.
hp2::recovered::DampingSuitability Smokeable::CheckDampingSuitability(){
 auto& m=memory_;const auto rigid=m.word(address_+0x60);
 // E-0110-F18: original VMUL.xyz/VADDY.x/VADDZ.x, then SQRT.S.
 // W is loaded but never enters XYZ length. Existing finite-range PC carriers
 // apply only to known finite operands; upstream NaN/Inf remains a hard guard.
 const auto length=[&](std::uint32_t p,const char* mul,const char* add_y,const char* add_z,const char* sqrt){const auto v=m.vector(p);
  for(unsigned lane=0;lane<3;++lane)if(!std::isfinite(v[lane])){const auto bits=[](float value){std::uint32_t word;std::memcpy(&word,&value,4);return std::to_string(word);};std::string why="Smokeable damping nonfinite input: object="+std::to_string(address_)+" rigid="+std::to_string(rigid)+" vector="+std::to_string(p)+" lane="+std::to_string(lane);for(unsigned i=0;i<4;++i)why+=" input"+std::to_string(i)+"_bits="+bits(v[i])+" momentum"+std::to_string(i)+"_bits="+bits(m.scalar(rigid+0x60+4*i))+" angular_momentum"+std::to_string(i)+"_bits="+bits(m.scalar(rigid+0x70+4*i));why+=" inverse_mass_bits="+bits(m.scalar(rigid+0x11c));for(unsigned i=0;i<16;++i)why+=" rotation"+std::to_string(i)+"_bits="+bits(m.scalar(rigid+0x20+4*i))+" inertia_inverse"+std::to_string(i)+"_bits="+bits(m.scalar(rigid+0x120+4*i));hp2::not_implemented("0x0021C210",why.c_str());}
  const float x=::port::driving::ee_mul_s_finite(v[0],v[0],mul),y=::port::driving::ee_mul_s_finite(v[1],v[1],mul),z=::port::driving::ee_mul_s_finite(v[2],v[2],mul);const float xy=::port::driving::ee_vadd_finite(x,y,add_y),square=::port::driving::ee_vadd_finite(xy,z,add_z);return ::port::driving::ee_sqrt_s(square,false,sqrt).value;};
 const float speed=length(rigid+0x90,"0x0021C218","0x0021C21C","0x0021C220","0x0021C230"),angular=length(rigid+0xa0,"0x0021C238","0x0021C23C","0x0021C240","0x0021C250"),maximum=m.scalar(0x325f8c);
 if(!(speed<=maximum)||!(angular<=maximum)||!m.byte(address_+0x12))return hp2::recovered::DS_NO_DAMP;
 if(!(speed<=.25f)||!(angular<=.25f))return hp2::recovered::DS_DAMP;
 const float elapsed=float(std::int32_t(m.word(0x327cf8)-m.word(address_+0x124)))*m.scalar(0x325f90);
 return elapsed>2.f?hp2::recovered::DS_DAMP:hp2::recovered::DS_DOUSE;
}
// Debug0021C300, E-0099-W83. Source PhysicsSystem calls remain concrete;
// external bindings are only Count, its actual delta and explicit stack-W policy.
void Smokeable::ComputeAuxiliaries(const ::port::race::PhysicsConstructionServices& service){
 auto& m=memory_;::port::driving::native::InitServices binding;
 if(service.ticker)binding.Sub_00250F38=[&service]{return std::uint64_t(service.ticker());};
 binding.Sub_00250F48=[&m](std::uint64_t start,std::uint64_t end){const float value=bGetTickerDifference(m,std::uint32_t(start),std::uint32_t(end));std::uint32_t bits;std::memcpy(&bits,&value,4);return bits;};
 binding.Field_00137938_stack_w=service.port_stack_vector_lane;binding.contact=service.contact;
 hp2::recovered::PhysicsSystem physics(m,address_+0x100);physics.ComputeAuxiliaries(binding);
 const auto damping=CheckDampingSuitability();const auto rigid=m.word(address_+0x60);
 if(damping==hp2::recovered::DS_DAMP){for(auto offset:{0x60u,0x70u}){auto v=m.vector(rigid+offset);for(auto& f:v)f=f*m.scalar(0x325f94);m.vector(rigid+offset,v);}}
 else if(damping==hp2::recovered::DS_DOUSE){if(!service.port_stack_vector_lane)hp2::not_implemented("0x0021C3AC","explicit original dousing-vector stack W policy is required");for(auto offset:{0x60u,0x70u}){for(unsigned i=0;i<3;++i)m.word(rigid+offset+4*i,0);m.word(rigid+offset+12,*service.port_stack_vector_lane);}}
 if(damping==hp2::recovered::DS_DAMP||damping==hp2::recovered::DS_DOUSE)physics.ComputeAuxiliaries(binding);
}
// Debug0021B430, E-0099-W83. Full source matrix/vector lanes, actual state,
// virtual auxiliaries and emitter matrices; independent unwritten W lanes use
// the declared PC policy, never an inferred original zero.
void Smokeable::SetInitialPosition(std::uint32_t matrix,std::uint32_t velocity,const ::port::race::PhysicsConstructionServices& service){
 auto& m=memory_;const auto a=address_;if(!matrix)hp2::not_implemented("0x0021B450","source initial matrix is dereferenced before its later optional-worldobject gate");
 if(!service.port_stack_vector_lane)hp2::not_implemented("0x0021B510","explicit original initial angular-vector stack W policy is required");
 ::port::driving::Memory::Temporary local(m,128);const auto p=local.address(),rotation=p+0x10,zero=p+0x50,momentum=p+0x60,cog=p+0x70;
 m.copy(p,matrix+0x30,16);m.copy(rotation,matrix,64);for(unsigned i=0;i<3;++i){m.word(rotation+0x30+4*i,0);m.word(zero+4*i,0);m.word(momentum+4*i,0);}m.scalar(rotation+0x3c,1);m.word(zero+12,*service.port_stack_vector_lane);m.scalar(momentum+12,1);
 const auto rigid=m.word(a+0x60);if(velocity){auto value=m.vector(velocity);for(auto& f:value)f=f*m.scalar(rigid+0x118);m.vector(momentum,value);}
 ::port::driving::SolverMathContext math{m.vector(0x317b20)};::port::driving::SolverVector transformed;
 hp2::recovered::bMulMatrix(transformed,m.matrix(rotation),m.vector(m.word(a+0x110)+0xa0),math);m.vector(cog,transformed);
 auto position=m.vector(p);for(unsigned i=0;i<4;++i)position[i]=position[i]+transformed[i];m.vector(p,position);
 hp2::recovered::RigidBody(m,rigid).SetState(p,rotation,momentum,zero);
 m.copy(a+0x410,matrix,64);m.word(a+0x3fc,1);m.copy(a+0xb0,p,16);m.copy(a+0x70,rotation,64);m.copy(a+0xc0,zero,16);m.word(a+0x64,m.word(0x325f78));
 const auto table=m.word(a+0x10c),target=m.word(table+0x14),self=a+0x100+std::int32_t(m.signed_half(table+0x10));
 if(target!=0x21c300)hp2::not_implemented("0x0021B59C","original initial-position auxiliaries virtual target is not reconstructed");
 Smokeable(m,self).ComputeAuxiliaries(service);UpdatePosition();
 for(auto offset:{0x450u,0x458u}){const auto head=a+offset;for(auto node=m.word(head);node!=head;node=m.word(node))m.copy(m.word(node+8)+0x30,matrix,64);}
}
}

#include "game_core/indep/src/collision.hpp"
#include "game_core/port/driving/ee_scalar.hpp"
namespace hp2::debug {
// Debug0021BF78, E-0099-W117; source retains the dynamic list membership.
void Smokeable::PutToSleep(){memory_.byte(address_+0x462,0);memory_.byte(address_+0x13,0);memory_.word(0x328b30,memory_.word(0x328b30)-1u);TopologyCoordinate(memory_,address_+0x12c).ClearInterestBBox();}
// Debug0021BE48, E-0099-W117; signed wrapped Timer differences and strict gates.
void Smokeable::ConsiderSleeping(){
 auto& m=memory_;const auto now=m.word(0x327cf8);const float scale=m.scalar(0x325f88);
 if(!(2.f<float(std::int32_t(now-m.word(address_+0x124)))*scale)||!m.signed_byte(address_+0x12))return;
 const auto collision=GetCollision(0);if(!collision)hp2::not_implemented("0x0021BEA8","source nonzero collision count requires actual Collision0 owner");
 if(!(.5f<float(std::int32_t(now-m.word(collision+8)))*scale))return;
 const auto rigid=m.word(address_+0x60);
 const auto magnitude=[&](std::uint32_t p,const char* instruction){const auto v=m.vector(p);return ::port::driving::ee_sqrt_s((v[0]*v[0]+v[1]*v[1])+v[2]*v[2],false,instruction).value;};
 if(magnitude(rigid+0x90,"0x0021BEFC")<1.5f&&magnitude(rigid+0xa0,"0x0021BF38")<4.f)PutToSleep();
}
// Debug0021C3F0, E-0099-W117; actual Collision and Car pointers, signed byte loop.
int Smokeable::IsTouchingACar(){const int count=memory_.signed_byte(address_+0x12);for(int i=0;i<count;++i){const auto collision=GetCollision(i);if(!collision)hp2::not_implemented("0x0021C430","source Smokeable collision index requires actual Collision owner");std::uint32_t first,second;Collision(memory_,collision).GetCollidingCars(first,second);if(first||second)return 1;}return 0;}
// Debug0021B888, E-0099-W117. MIN.S numeric domain is explicitly ordinary;
// actual shared topology receives original position/velocity pointers' XY.
void Smokeable::PositionChanged(){
 auto& m=memory_;const float base=m.scalar(address_+0xd0),radius=m.scalar(address_+0xd8);
 const auto ordinary=[](float f){return std::isfinite(f)&&(f==0||std::isnormal(f));};
 if(!ordinary(base)||!ordinary(radius))hp2::not_implemented("0x0021B8BC","source Smokeable MIN.S dimensions outside ordinary EE scalar domain");
 const float extent=base==0&&radius==0?(std::signbit(base)||std::signbit(radius)?-0.f:0.f):(base<radius?base:radius);const auto velocity=::port::race::detail::smokeable_pose(m,address_,0x20,"0x0021B8C0");const auto v=m.vector(velocity);
 const float speed=::port::driving::ee_sqrt_s((v[0]*v[0]+v[1]*v[1])+v[2]*v[2],false,"0x0021B8E0").value*m.scalar(0x325f7c);
 if(!ordinary(speed))hp2::not_implemented("0x0021B908","source Smokeable MIN.S speed outside ordinary EE scalar domain");
 const auto position=::port::race::detail::smokeable_pose(m,address_,0x18,"0x0021B90C");const float interest=extent+(speed<6.f?speed:6.f);
 const auto current_velocity=::port::race::detail::smokeable_pose(m,address_,0x20,"0x0021B928");const std::array<float,2> motion{{m.scalar(current_velocity),m.scalar(current_velocity+4)}};
 TopologyCoordinate(m,address_+0x12c).SetInterestBBox({{m.scalar(position),m.scalar(position+4)}},interest,&motion);UpdatePosition();
}
// Debug0021B968, E-0099-W117. All matrix lanes copied; source scalar XYZ
// emitter velocity stores explicitly clear W, unlike an unwritten stack lane.
void Smokeable::ServiceEmitters(std::uint32_t matrix){
 auto& m=memory_;if(!m.signed_byte(address_+0x462))return;
 const auto velocity=::port::race::detail::smokeable_pose(m,address_,0x20,"0x0021B994");
 const auto store_velocity=[&](std::uint32_t emitter){for(unsigned i=0;i<3;++i)m.word(emitter+0x70+4*i,m.word(velocity+4*i));m.word(emitter+0x7c,0);};
 const auto dynamic=address_+0x458;for(auto node=m.word(dynamic);node!=dynamic;node=m.word(node)){const auto emitter=m.word(node+8);m.copy(emitter+0x30,matrix,64);store_velocity(emitter);}
 if(m.signed_byte(address_+0x460)==1){const auto anchor=address_+0x450;for(auto node=m.word(anchor);node!=anchor;node=m.word(node))store_velocity(m.word(node+8));m.byte(address_+0x460,2);}
}
// Debug0021BBD8, E-0099-W117. The original basic-class body is JR RA/NOP;
// other virtual Warhead targets are deliberately not mapped to this no-op.
void Smokeable::MaybeExplodeWarhead(){}
// Debug0021BA78, E-0099-W117. Actual virgin recreation/basic expiry cleanup;
// nondefault virtual warhead/destructor targets retain exact child guards.
void Smokeable::CheckTimers(){
 auto& m=memory_;const auto a=address_,awoken=m.word(a+0x120);const bool set=awoken&&awoken!=0x7fffffffu;
 const auto destroy=[&](const char* call){const auto table=m.word(a+0x50),target=m.word(table+0xc),self=a+std::int32_t(m.signed_half(table+8));if(target!=0x21b0d0)hp2::not_implemented(call,"source Smokeable timer requires unreconstructed destructor target");Smokeable(m,self).Sub_0021B0D0(3);};
 if(m.word(a+0x128)&&!set&&!m.signed_byte(a+0x10)&&2.f<float(std::int32_t(m.word(0x327cf8)-m.word(a+0x11c)))*m.scalar(0x325f80)){
  const auto virgin=bOMalloc(m,m.word(0x328b28));VirginSmokeable(m,virgin).Sub_0021A9F8(m.word(a+0x128));if(a)destroy("0x0021BB1C");return;
 }
 if(!set)return;
 const float lifetime=m.scalar(m.word(a+0x110)+0xb0);
 if(!std::isfinite(lifetime)||(lifetime!=0&&!std::isnormal(lifetime)))hp2::not_implemented("0x0021BB58","source Smokeable lifetime comparison outside ordinary EE domain");
 if(lifetime==0||!(lifetime<=float(std::int32_t(m.word(0x327cf8)-awoken))*m.scalar(0x325f84)))return;
 const auto table=m.word(a+0x50),target=m.word(table+0xdc),self=a+std::int32_t(m.signed_half(table+0xd8));
 if(target!=0x21bbd8)hp2::not_implemented("0x0021BBA0","source Smokeable expiry requires unreconstructed Warhead virtual target");Smokeable(m,self).MaybeExplodeWarhead();if(a)destroy("0x0021BBBC");
}
// Debug0021B688, E-0099-W117. Real PhysicsSystem integration and existing
// auxiliary/orthonormalizer children; services bind hardware/context only.
void Smokeable::Move(float timestep,const ::port::race::PhysicsConstructionServices& service){
 auto& m=memory_;const auto a=address_;BeginMove(timestep);
 if(m.signed_byte(a+0x462)){
  Calibrate();const auto rigid=m.word(a+0x60);const float maximum=m.scalar(0x328b48);
  if(!std::isnormal(maximum)||maximum<=0||!std::isfinite(timestep)||(timestep!=0&&!std::isnormal(timestep)))hp2::not_implemented("0x0021B6DC","source Smokeable timestep DIV operands outside ordinary positive-maximum domain");
  const float ratio=timestep/maximum;m.word(rigid+0x110,1);
  if(!std::isfinite(ratio)||ratio<0||ratio>=2147483520.f||(ratio!=0&&!std::isnormal(ratio)))hp2::not_implemented("0x0021B6E4","source Smokeable CVT/step count outside supported nonnegative signed range");
  const auto steps=std::int32_t(ratio)+1;
  // Source21B704 divides in an always-executed delay slot, even if a loop
  // would be skipped. Unsupported nonpositive/zero step domains are not skipped.
  if(steps<=0)hp2::not_implemented("0x0021B704","source Smokeable substep DIV requires resolved nonpositive-step EE domain");
  if(steps>0x1000000)hp2::not_implemented("0x0021B6FC","source Smokeable step CVT.S.W exceeds exact integer-to-float domain");
  const float duration=timestep/float(steps);
  if(!std::isfinite(duration)||(timestep!=0&&!std::isnormal(duration)))hp2::not_implemented("0x0021B704","source Smokeable substep DIV outside supported normal/explicit-zero result domain");
  ::port::driving::native::IntegrationCalls calls;
  calls.derivative=[](auto& memory,auto object,auto output){const auto table=memory.word(object+8),target=memory.word(table+0x3c),adjusted=object+std::int32_t(memory.signed_half(table+0x38));if(target!=0x19adf8)hp2::not_implemented("0x00199F98","source Smokeable component derivative target is not reconstructed");hp2::recovered::RigidBody(memory,adjusted).GetDerivative(output);};
  calls.after=[&service](auto& memory,auto system){const auto table=memory.word(system+0xc),target=memory.word(table+0x14),adjusted=system+std::int32_t(memory.signed_half(table+0x10));if(target!=0x21c300)hp2::not_implemented("0x00199BB8","source Smokeable post-Euler auxiliaries target is not reconstructed");Smokeable(memory,adjusted).ComputeAuxiliaries(service);};
  for(std::int32_t i=0;i<steps;++i){hp2::recovered::PhysicsSystem(m,a+0x100).IntegrateEuler(duration,calls);hp2::recovered::eOrthonormalizeMatrix(m,m.word(a+0x60)+0x20);}
  ConsiderSleeping();PositionChanged();if(IsTouchingACar())m.word(a+0x124,m.word(0x327cf8));
 }
 const auto rigid=m.word(a+0x60);::port::driving::SolverVector offset;
 hp2::recovered::bMulMatrix(offset,m.matrix(rigid+0x20),m.vector(m.word(a+0x110)+0xa0),{m.vector(0x317b20)});
 auto matrix=m.matrix(rigid+0x20);const auto position=m.vector(rigid+0x10);for(unsigned i=0;i<4;++i)matrix[12+i]=position[i]-offset[i];matrix[15]=1;
 const auto table=m.word(a+0x50),target=m.word(table+0xbc),self=a+std::int32_t(m.signed_half(table+0xb8));if(target!=0x21c8d0)hp2::not_implemented("0x0021B7E4","source Smokeable IsBlinking virtual target is not reconstructed");
 if(!Smokeable(m,self).IsBlinking()||(m.word(0x327d10)&1)){m.matrix(a+0x410,matrix);m.word(a+0x3fc,1);}else m.word(a+0x3fc,0);
 for(unsigned i=0;i<4;++i)matrix[12+i]=matrix[12+i]+offset[i];matrix[15]=1;
 ::port::driving::Memory::Temporary local(m,64);m.matrix(local.address(),matrix);ServiceEmitters(local.address());CheckTimers();
}
}

namespace hp2::debug {
// Debug0021C880,E-0110-F09, original smokeable.cpp leaf.
float Smokeable::GetMassInverse()const{if(memory_.scalar(address_+0x114)!=0)return 0;return memory_.scalar(memory_.word(address_+0x60)+0x11c);}
// Debug0021C7F8/0021C808/0021C828/0021C850/0021C8E8; E-0110-E03.
// Exact source resource fields, anchor equality and signed Awake byte gate.
float Smokeable::GetFriction()const{return memory_.scalar(memory_.word(address_+0x110)+0xc8);}
float Smokeable::GetBounciness()const{return memory_.scalar(memory_.word(address_+0x110)+0xbc);}
std::uint32_t Smokeable::GetInertiaTensorInverse()const{return memory_.scalar(address_+0x114)==0?memory_.word(address_+0x60)+0x120:0x311620;}
void Smokeable::ApplyImpulse(std::uint32_t impulse,std::uint32_t arm){if(memory_.signed_byte(address_+0x462))hp2::recovered::RigidBody(memory_,memory_.word(address_+0x60)).ApplyImpulse(memory_.vector(impulse),memory_.vector(arm));}
}

namespace hp2::debug {
// Debug0021BBE0,E-0110-F13. Actual source list ordering and activation byte.
void Smokeable::ActivateAnchorEmitters(){auto& m=memory_;const auto head=address_+0x450;for(auto node=m.word(head);node!=head;node=m.word(node))AcidEffectSystem(m,0x2e6f40).AddAutoEmitter(m.word(node+8));m.byte(address_+0x460,1);}
// Debug0021BC48,E-0110-F13. Sound is an explicit optional PC presentation
// boundary before entry; anchor response, activation and strength updates remain.
float Smokeable::ConsiderImpulse(std::uint32_t impulse,std::uint32_t arm,bool disable_collision_sound){
 auto& m=memory_;const auto a=address_;float factor=1;
 const auto v=m.vector(impulse);
 if(!m.signed_byte(a+0x462)){
  const float strength=m.scalar(a+0x114);
  if(strength!=0){
   const auto center=m.vector(m.word(a+0x110)+0xa0),r=m.vector(arm);
   const std::array<float,3> anchor{{r[0]+center[0],r[1]+center[1],r[2]+center[2]}};
   const std::array<float,3> moment{{anchor[1]*v[2]-anchor[2]*v[1],anchor[2]*v[0]-anchor[0]*v[2],anchor[0]*v[1]-anchor[1]*v[0]}};
   const float moment_length=std::sqrt((moment[0]*moment[0]+moment[1]*moment[1])+moment[2]*moment[2]);
   const float impulse_length=std::sqrt((v[0]*v[0]+v[1]*v[1])+v[2]*v[2]);
   if(!std::isfinite(moment_length)||!std::isfinite(impulse_length))hp2::not_implemented("0x0021BCFC","Exceptional EE Smokeable magnitude/MAX operands are not reconstructed");
   const float magnitude=impulse_length<moment_length?moment_length:impulse_length;
   if(magnitude<strength)return 1;
   if(magnitude==0)hp2::not_implemented("0x0021BD14","Exceptional zero Smokeable anchor division is not reconstructed");
   factor=strength/magnitude;
   if(!disable_collision_sound&&m.word(0x326f40))hp2::not_implemented("0x001E1B68","collision_sound.PlaySmokeable sound playback children are not reconstructed");
   ::port::driving::Memory::Temporary temp(m,32);const auto response=temp.address(),anchor_arm=response+16;std::array<float,4> out,offset;
   for(unsigned i=0;i<4;++i){out[i]=v[i]*(-factor);offset[i]=(i==3?1.f:0.f)-center[i];}m.vector(response,out);m.vector(anchor_arm,offset);
   Awaken();m.scalar(a+0x114,0);
   const auto table=m.word(a+0x50),target=m.word(table+0x84),self=a+std::int32_t(m.signed_half(table+0x80));
   if(target!=0x21c8e8)hp2::not_implemented("0x0021BD80","Unrecovered Smokeable anchor ApplyImpulse virtual target");
   Smokeable(m,self).ApplyImpulse(response,anchor_arm);ActivateAnchorEmitters();
  }else Awaken();
 }
 const float strength=m.scalar(a+0x118);
 if(strength!=0){const float magnitude=factor*((std::abs(v[0])+std::abs(v[1]))+std::abs(v[2]));
  if(strength<=magnitude){m.scalar(a+0x118,-1);if(magnitude==0)hp2::not_implemented("0x0021BE0C","Exceptional zero Smokeable breaking division is not reconstructed");factor=((-factor)*strength)/magnitude;}
  else m.scalar(a+0x118,strength-magnitude*0.5f);
 }return factor;
}
}
