// Debug source marker ../indep/src/clans.cpp. E-0099-W06 captures the ECOFF
// FDR/SYMR/marker/string via read-only MCP, separately from function recovery.
#include "game_core/port/race/world_construction.hpp"
#include "game_core/inc/bSlotPool.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/indep/inc/Clans.hpp"
#include "game_core/indep/inc/Skids.hpp"
#include "game_core/port/driving/projections.hpp"
#include <algorithm>
#include <cmath>

namespace hp2::debug {
// Debug001A90F0, E-0099-W06. Explicit original process-static list construction.
void Sub_001A90F0(::port::driving::Memory& memory){memory.word(0x327e90,0x327e90);memory.word(0x327e94,0x327e90);}
// Debug001A8C98, E-0099-W06; Debug SHA256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
void InitClans(::port::driving::Memory& memory){
    if(memory.word(0x00327e98)==0)
        memory.word(0x00327e98,bNewSlotPool(memory,0x50,0x28,0x002addc8,0x10));
}
}

namespace hp2::debug {
// Debug001A8D20 ->001A8CD0/001A8FE0, E-0099-W18. Fresh World has no clans.
void CloseClans(::port::driving::Memory& m){
 if(!m.word(0x327e98))return;
 FlushClans(m);
 bDeleteSlotPool(m,m.word(0x327e98));m.word(0x327e98,0);
}
}

// Debug001A8F90/8FE0/8D50/8CD0, E-0099-W71; exact original clans.cpp.
// SHA256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
namespace hp2::debug {
namespace {
// Port-only logical-memory/list adapters; not additional recovered functions.
void clan_unlink(::port::driving::Memory&m,std::uint32_t p){const auto n=m.word(p),b=m.word(p+4);m.word(b,n);m.word(n+4,b);}
void clan_front(::port::driving::Memory&m,std::uint32_t p){const auto h=0x327e90u,n=m.word(h);m.word(h,p);m.word(n+4,p);m.word(p+4,h);m.word(p,n);}
}
void Clan::Sub_001A8F90(std::uint32_t p,std::uint32_t hash){const auto h=address_+8;m_.word(h,h);m_.word(h+4,h);m_.copy(address_+0x20,p,16);m_.word(address_+0x10,hash);m_.copy(address_+0x30,p,16);m_.copy(address_+0x40,p,16);/* original leaves LastUpdateTime+14 untouched */}
void Clan::Sub_001A8FE0(int flags){const auto h=address_+8;while(m_.word(h)!=h)DeleteThisSkid(m_,m_.word(m_.word(h)+8));if(flags&1)SlotPool(m_,m_.word(0x327e98)).Free(address_);}
void Clan::ExpandBoundingBox(std::uint32_t lo,std::uint32_t hi){for(unsigned i=0;i<4;++i){const auto at=i*4;const float a=m_.scalar(address_+0x30+at),b=m_.scalar(lo+at),c=m_.scalar(address_+0x40+at),d=m_.scalar(hi+at);if(!std::isfinite(a)||!std::isfinite(b)||!std::isfinite(c)||!std::isfinite(d))hp2::not_implemented("0x00252AB0","Exceptional original full-lane VU Clan bounding box MIN/MAX");m_.scalar(address_+0x30+at,std::min(a,b));m_.scalar(address_+0x40+at,std::max(c,d));}}
std::uint32_t GetClan(::port::driving::Memory&m,std::uint32_t position){const auto x=::port::driving::cvt_w_s(m.scalar(position)*65536.f),y=::port::driving::cvt_w_s(m.scalar(position+4)*65536.f);const auto hash=(std::uint32_t(y>>22)<<16)+(std::uint32_t(x>>22)&0xffff);const auto head=0x327e90u,pool=m.word(0x327e98);if(!pool||!m.word(head))hp2::not_implemented("0x001A8C98","Original Clan static list and pool startup are required");for(auto p=m.word(head);p!=head;p=m.word(p))if(m.word(p+0x10)==hash){clan_unlink(m,p);m.word(p+0x14,m.word(0x327d00));clan_front(m,p);return p;}if(m.word(pool+0x18)==m.word(pool+0x2c)){const auto old=m.word(head+4);if(old==head)hp2::not_implemented("0x001A8E2C","Original full Clan pool has no published LRU owner");clan_unlink(m,old);Clan(m,old).Sub_001A8FE0(3);}const auto p=bOMalloc(m,pool);Clan(m,p).Sub_001A8F90(position,hash);clan_front(m,p);return p;}
void FlushClans(::port::driving::Memory&m){const auto h=0x327e90u;while(m.word(h)!=h){const auto p=m.word(h);clan_unlink(m,p);Clan(m,p).Sub_001A8FE0(3);}}
}
