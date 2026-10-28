#include "game_core/indep/inc/World.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/indep/src/drivetarget.hpp"
#include "game_core/TrafficTeleporter.hpp"
#include <array>
#include <cstdio>
#include <stdexcept>
#include <vector>
namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void empty(port::driving::Memory& memory,std::uint32_t head){memory.word(head,head);memory.word(head+4,head);}
void link(port::driving::Memory& memory,std::uint32_t head,const std::vector<std::uint32_t>& cars){
    empty(memory,head);auto previous=head;
    for(auto car:cars){memory.word(previous,car);memory.word(car+4,previous);previous=car;}
    memory.word(previous,head);memory.word(head+4,previous);
}
}
int main(){
    port::driving::Memory memory;const auto world=memory.allocate(0x1730);hp2::debug::World state(memory,world);
    memory.map(0x311f78,0x310);memory.map(0x325f40,4);memory.scalar(0x325f40,.001f);
    const auto target=memory.allocate(0x210);
    for(unsigned i=0;i<0x210;++i)memory.byte(target+i,77);
    hp2::debug::DriveTarget drive_target(memory,target);
    require(memory.signed_byte(target+0x204)==-1 && memory.word(target+0x1fc)==0
        && memory.word(target+0x1f8)==0 && memory.byte(target)==0,"DriveTarget constructor must call original Clear after zeroing");
    memory.byte(target,3);memory.scalar(target+0x200,100);drive_target.Clear(0,0);
    require(memory.byte(target)==0 && memory.scalar(target+0x200)==0,"zero-distance drive target clear differs");
    memory.byte(target,3);drive_target.Clear(0,-1);require(memory.byte(target)==0,"negative-distance target clear differs");
    memory.byte(target,3);require(!hp2::guard([&]{drive_target.Clear(0,10);}).empty() && memory.byte(target)==3,
        "untranslated positive-distance target branch must fail before partial clear");
    memory.byte(target,255);drive_target.Clear(0,10);require(memory.byte(target)==0,"signed negative point-count clear branch differs");
    const auto teleporter=memory.allocate(0x58);memory.scalar(0x311f78+0x10,1);
    std::vector<unsigned> route_calls;
    const port::race::RoutePointCount routes=[&](unsigned route)->std::optional<std::uint32_t>{route_calls.push_back(route);
        if(route==51)return std::nullopt;
        if(route==50)return 3;
        if(route==52)return 5;
        if(route==60)return 9;
        return std::nullopt;};
    hp2::debug::TrafficTeleporter teleport(memory,teleporter,routes);
    require(route_calls.size()==20 && route_calls.front()==50 && route_calls[9]==59 && route_calls[10]==60 && route_calls.back()==69,
        "TrafficTeleporter must traverse original forward/backward route ranges");
    require(memory.word(teleporter+8)==3 && memory.word(teleporter+12)==3 && memory.word(teleporter+16)==8
        && memory.word(teleporter+0x30)==9 && memory.scalar(teleporter)==20 && memory.word(teleporter+4)==0,
        "traffic route cumulative counts or constructor minimum differ");
    teleport.SetThinkTime();require(memory.scalar(teleporter)==1,"think-time setter must not apply constructor minimum");
    memory.scalar(0x311f78+0x10,-1);hp2::debug::TrafficTeleporter slow(memory,teleporter,routes);
    require(memory.scalar(teleporter)==31,"negative density original think-time math changed");
    empty(memory,world);empty(memory,world+8);state.BuildCarTable();
    require(memory.word(world+0x1c)==0 && memory.word(world+0x28)==0 && state.GetCarByNumber(0)==0,"empty original World table differs");
    std::vector<std::uint32_t> cars;
    const std::array<int,9> types{3,4,1,2,5,3,0,2,1};
    for(unsigned i=0;i<types.size();++i){
        // Explicit fixture: only accessed Car header + DriverInfo fields, not a constructed Car/World.
        const auto car=memory.allocate(0x18),driver=memory.allocate(24);
        memory.word(car+0x14,driver);memory.word(driver+8,types[i]);memory.half(car+8,0xeeee);
        memory.half(car+0xa,static_cast<std::uint16_t>(int(i)-7));cars.push_back(car);
    }
    link(memory,world,cars);
    for(unsigned i=0;i<14;++i)memory.word(world+0x2c+4*i,0xfeedbeef);
    state.BuildCarTable();
    const std::array<unsigned,8> expected{2,8,3,7,1,4,0,5};
    require(memory.word(world+0x1c)==8 && memory.word(world+0x20)==2 && memory.word(world+0x24)==4
        && memory.word(world+0x28)==6,"player/AI/cop/traffic partition boundaries differ");
    for(unsigned i=0;i<expected.size();++i){require(memory.word(world+0x2c+4*i)==cars[expected[i]]
        && memory.half(cars[expected[i]]+8)==i,"stable original category traversal/CarIndex stores differ");}
    require(memory.half(cars[6]+8)==0xeeee && memory.word(world+0x2c+4*8)==0xfeedbeef,"unknown type or stale trailing table storage was overwritten");
    require(state.GetCarByNumber(-7)==cars[0] && state.GetCarByNumber(1)==cars[8] && state.GetCarByNumber(999)==0,
        "GetCarByNumber must compare signed DriverNumber, not CarIndex");
    const auto probe=memory.allocate(16);const auto before=memory.footprint();
    for(int i=0;i<30;++i)state.BuildCarTable();
    const auto after=memory.footprint();
    require(before.regions==after.regions && before.payload_bytes==after.payload_bytes && before.owned_allocations==after.owned_allocations,
        "table rebuild grew shared memory storage");
    require(memory.allocate(16)==probe+32,"World table rebuild introduced persistent logical allocations");
    memory.word(world+0x178,0xaabbccdd);memory.scalar(world+0x180,123);memory.word(world+0x194,0x123456);
    memory.scalar(world+0x184,.5f);memory.scalar(world+0x188,9);memory.scalar(world+0x17c,10);
    memory.word(world+0x18c,99);memory.word(world+0x190,88);state.ResetTimeScale();
    require(memory.scalar(world+0x184)==1 && memory.scalar(world+0x188)==0 && memory.scalar(world+0x17c)==0
        && memory.word(world+0x18c)==0 && memory.word(world+0x190)==0,"original time-scale reset stores differ");
    require(memory.word(world+0x178)==0xaabbccdd && memory.scalar(world+0x180)==123 && memory.word(world+0x194)==0x123456,
        "time reset changed unrelated RNG/waste/teleporter state");
    memory.word(world+0x1c,77);memory.word(cars.back(),cars.front());
    require(!hp2::guard([&]{state.BuildCarTable();}).empty() && memory.word(world+0x1c)==77,
        "malformed circular list must fail before table mutation");
    link(memory,world,cars);memory.word(world+8,0);
    require(!hp2::guard([&]{state.BuildCarTable();}).empty(),"null AI list link must fail explicitly");
    empty(memory,world+8);
    cars.clear();
    for(int i=0;i<15;++i){const auto car=memory.allocate(0x18),driver=memory.allocate(24);
        memory.word(car+0x14,driver);memory.word(driver+8,1);cars.push_back(car);}
    link(memory,world,cars);
    require(!hp2::guard([&]{state.BuildCarTable();}).empty(),"World table overflow must not write into adjacent fields");
    std::puts("PASS original World category order, shared links/indices, lookup/time reset and malformed-list guards");
}
