// TASK-0042 E-0042-S001: exact Debug source marker(s): src\bSlotPool.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): SlotPool::CleanupExpandedSlotPools() [0x002C0450]
#include "game_core/inc/bSlotPool.hpp"
#include <algorithm>
#include <cstdio>
#include <limits>
#include <stdexcept>

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x00257F28; E-0019-N03; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/memory/00257f28.md.
void SlotPool::CleanupExpandedSlotPools() {
    hp2::not_implemented("0x00257F28", "SlotPool::CleanupExpandedSlotPools (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::debug {
using port::driving::Memory;
// Debug00251110, E-0099-W05. Host allocation replaces bMalloc's device arena;
// the original header, free-list order, flags and counters remain in Memory.
std::uint32_t SlotPool::NewSlotPool(Memory& m,int size,int count,std::uint32_t name,int pool){
    if(size<4 || count<1 || std::uint64_t(size)*std::uint64_t(count)>0xffffffcfu)
        throw std::invalid_argument("Invalid original slot-pool allocation domain");
    const auto at=m.allocate(std::uint32_t(size)*std::uint32_t(count)+0x30);
    m.word(at+0x28,size);m.word(at+0x20,pool);m.word(at+0xc,name);
    m.word(at+0x2c,count);m.word(at+0x24,count);m.word(at+0x10,0);m.word(at+8,0);
    m.word(at+0x18,0);m.word(at+0x1c,0);m.word(at+0x14,15);
    SlotPool(m,at).FlushSlotPool();return at;
}
// Debug002511C8, E-0099-W05. Does NOT reset allocation/high-water counters.
void SlotPool::FlushSlotPool(){
    for(auto pool=address_;pool;pool=memory_.word(pool+8)){
        const int count=memory_.signed_word(pool+0x24),size=memory_.signed_word(pool+0x28);
        if(count<1 || size<4)throw std::invalid_argument("Invalid original pool header");
        auto slot=pool+0x30;memory_.word(pool+0x10,slot);
        for(int i=1;i<count;++i){const auto next=slot+size;memory_.word(slot,next);slot=next;}
        memory_.word(slot,0);
    }
}
// Debug00251238, E-0099-W05. Expansion prepends a chain and inherits root flags.
void SlotPool::ExpandSlotPool(int count){
    auto& m=memory_;const auto root=address_;
    if(m.word(root+0x14)&SLOTPOOL_FLAG_WARN_IF_OVERFLOW)
        std::fputs("port: original SlotPool expansion warning\n",stderr);
    const auto extra=NewSlotPool(m,m.signed_word(root+0x28),count,m.word(root+0xc),m.signed_word(root+0x20));
    m.word(extra+8,m.word(root+8));m.word(root+8,extra);
    m.word(root+0x2c,m.word(root+0x2c)+std::uint32_t(count));
    m.word(extra+0x30+std::uint32_t(count-1)*m.word(root+0x28),m.word(root+0x10));
    m.word(extra+0x14,m.word(root+0x14));m.word(root+0x10,m.word(extra+0x10));m.word(extra+0x10,0);
}
// Debug00251500, E-0099-W05. Quarter of original root capacity plus one.
std::uint32_t SlotPool::Malloc(){
    auto& m=memory_;auto slot=m.word(address_+0x10);
    if(!slot && (m.word(address_+0x14)&SLOTPOOL_FLAG_OVERFLOW_IF_FULL)){
        ExpandSlotPool(m.signed_word(address_+0x24)/4+1);slot=m.word(address_+0x10);
    }
    if(slot){m.word(address_+0x10,m.word(slot));
        if(m.word(address_+0x14)&SLOTPOOL_FLAG_ZERO_ALLOCATED_MEMORY)
            for(int i=0;i<(m.signed_word(address_+0x28)>>2);++i)m.word(slot+4*i,0);
        const auto count=m.word(address_+0x18)+1;m.word(address_+0x18,count);
        if(m.signed_word(address_+0x1c)<static_cast<std::int32_t>(count))m.word(address_+0x1c,count);
    }
    return slot;
}
// Debug002515D0, E-0099-W05. No clearing and no expansion.
std::uint32_t SlotPool::FastMalloc(){
    auto& m=memory_;const auto slot=m.word(address_+0x10);
    if(slot){const auto count=m.word(address_+0x18)+1;
        m.word(address_+0x10,m.word(slot));m.word(address_+0x18,count);
        if(m.signed_word(address_+0x1c)<static_cast<std::int32_t>(count))m.word(address_+0x1c,count);
    }
    return slot;
}
// Debug00251608, E-0099-W05. LIFO free; payload after the link stays untouched.
void SlotPool::Free(std::uint32_t slot){
    memory_.word(slot,memory_.word(address_+0x10));memory_.word(address_+0x10,slot);
    memory_.word(address_+0x18,memory_.word(address_+0x18)-1u);
}
// Debug002510E0, E-0099-W83; original src/bSlotPool.cpp.
int bIsSlotPoolFull(const ::port::driving::Memory& memory,std::uint32_t pool){
    return memory.word(pool+0x18)==memory.word(pool+0x2c);
}
// Debug002512F8, E-0099-W05. Remove completely free expanded pools, retaining
// root free-list order, counts and high water. Valid-domain links are original.
void SlotPool::CleanupExpandedSlotPools(){
    auto& m=memory_;const auto root=address_;auto child=m.word(root+8);if(!child)return;
    for(auto p=child;p;p=m.word(p+8))m.word(p+0x18,m.word(p+0x24));
    for(auto slot=m.word(root+0x10);slot;slot=m.word(slot)){
        for(auto p=child;p;p=m.word(p+8)){
            const auto size=m.signed_word(p+0x28);if(size<=0)throw std::invalid_argument("Invalid slot size during cleanup");
            const auto delta=std::int64_t(slot)-p-0x30;const auto index=delta/size;
            if(index>=0 && index<m.signed_word(p+0x24)){m.word(p+0x18,m.word(p+0x18)-1u);break;}
        }
    }
    auto previous=root;
    for(auto p=child;p;){const auto next=m.word(p+8);
        if(m.word(p+0x18)==0){
            auto link=root+0x10;int removed=0;const auto size=m.signed_word(p+0x28),count=m.signed_word(p+0x24);
            for(auto slot=m.word(link);slot;){const auto following=m.word(slot);const auto index=(std::int64_t(slot)-p-0x30)/size;
                if(index>=0 && index<count){m.word(link,following);++removed;}else link=slot;
                slot=following;
            }
            if(removed!=count)throw std::logic_error("Original expanded slot count invariant failed");
            m.word(previous+8,next);m.word(root+0x2c,m.word(root+0x2c)-std::uint32_t(count));m.release(p);
        }else{previous=p;if(m.word(root+0x14)&SLOTPOOL_FLAG_WARN_IF_OVERFLOW)
            std::fputs("port: original SlotPool cleanup retains allocated expansion\n",stderr);}
        p=next;
    }
}
// Debug002517F0, E-0099-W05. The original process-static manager constructor.
void Sub_002517F0(Memory& m,std::uint32_t at){m.word(at+4,at+4);m.word(at+8,at+4);m.word(at,1);}
// Debug00250FE0/00251880, E-0099-W05. Manager rounds size to four bytes.
std::uint32_t bNewSlotPool(Memory& m,int size,int count,std::uint32_t name,int pool){
    if(size<1 || size>std::numeric_limits<int>::max()-3)throw std::invalid_argument("Invalid slot size");
    const auto at=SlotPool::NewSlotPool(m,(size+3)&~3,count,name,pool);
    constexpr std::uint32_t manager=0x316e48,head=manager+4;
    if(m.word(manager)==1){const auto previous=m.word(head+4);m.word(previous,at);m.word(head+4,at);m.word(at+4,previous);m.word(at,head);}
    else std::fputs("port: original SlotPoolManager not initialized warning\n",stderr);
    return at;
}
// Debug00251018/00251918/002511B0, E-0099-W05. Detach root then free chain.
void bDeleteSlotPool(Memory& m,std::uint32_t at){
    if(!at)return;
    if(m.word(0x316e48)!=0){const auto next=m.word(at),previous=m.word(at+4);m.word(previous,next);m.word(next+4,previous);m.word(at,0xb);m.word(at+4,0xb);}
    else std::fputs("port: original SlotPoolManager delete before initialization warning\n",stderr);
    if((m.word(at+0x14)&SLOTPOOL_FLAG_WARN_IF_NONEMPTY_DELETE) && m.signed_word(at+0x18)>0)
        std::fputs("port: original SlotPool nonempty deletion warning\n",stderr);
    while(at){const auto next=m.word(at+8);m.release(at);at=next;}
}
// Debug00251050, E-0099-W05.
std::uint32_t bOMalloc(Memory& m,std::uint32_t at){return SlotPool(m,at).Malloc();}
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): SlotPoolManager::NewSlotPool [0x002C04B0]
// Diagnostic text (not a callable signature): SlotPoolManager::SlotPoolManager() [0x002C04B0]
// Diagnostic text (not a callable signature): SlotPoolManager::DeleteSlotPool [0x002C0510]
#include "game_core/SlotPoolManager.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x002584B0; E-0019-N03; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/memory/002584b0.md.
void SlotPoolManager::NewSlotPool() {
    hp2::not_implemented("0x002584B0", "SlotPoolManager::NewSlotPool (signature and full body unresolved)");
}

// ELF 0x00258548; E-0019-N03; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/memory/00258548.md.
void SlotPoolManager::DeleteSlotPool() {
    hp2::not_implemented("0x00258548", "SlotPoolManager::DeleteSlotPool (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00257DF8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00257df8.md.
void SlotPool::FlushSlotPool() {
    hp2::not_implemented("0x00257DF8", "SlotPool::FlushSlotPool (signature and full body unresolved)");
}

// ELF 0x00258200; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00258200.md.
void SlotPool::FastMalloc() {
    hp2::not_implemented("0x00258200", "SlotPool::FastMalloc (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::debug {
// Debug002510F8, E-0099-W57; original src/bSlotPool.cpp wrapper and
// inc/bSlotPool.hpp inline count. SUBU low32 result interpreted as signed int.
std::int32_t bCountFreeSlots(const ::port::driving::Memory& m,std::uint32_t pool){return static_cast<std::int32_t>(m.word(pool+0x2c)-m.word(pool+0x18));}
}
