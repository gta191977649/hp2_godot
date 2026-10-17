#include "skid_services.hpp"
#include "game_core/port/not_implemented.hpp"
#include <stdexcept>
namespace port::driving {
void allocate_skid_arena(Memory&m,SkidArena&a){
    a.list=m.allocate(8);a.clan_list=m.allocate(8);a.clan_bbox=m.allocate(32);
    a.pool=m.allocate(0x110*128);a.used=m.allocate(128);a.nodes=m.allocate(16*128);
    for(auto head:{a.list,a.clan_list}){m.word(head,head);m.word(head+4,head);}
}
std::uint32_t allocate_skid_slot(Memory&m,const SkidArena&a){
    for(unsigned i=0;i<128;++i)if(!m.byte(a.used+i)){m.byte(a.used+i,1);return a.pool+i*0x110;}
    return 0;
}
void free_skid_slot(Memory&m,const SkidArena&a,std::uint32_t p){
    if(p<a.pool||(p-a.pool)%0x110||(p-a.pool)/0x110>=128)throw std::runtime_error("Invalid skid pool address");
    m.byte(a.used+(p-a.pool)/0x110,0);
}
void validate_skid_services(const SkidServices&s){if(!s.arena||!s.arena->list)hp2::not_implemented("0x001A8798","Skid pool and synthetic clan services are not bound");}
}
