#include "game_core/indep/inc/Car.hpp"
#include "game_core/indep/src/pursuit.hpp"
#include "game_core/indep/src/coputil.hpp"
#include "game_core/port/race/control_construction.hpp"
#include "game_core/port/race/physics_construction.hpp"
#include "game_core/port/runtime_warnings.hpp"
#include "game_core/indep/inc/CopChase.hpp"
// Exact Debug source marker ../indep/src/copmgr.cpp; E-0099-W11.
// NFSHP2.ELF SHA256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/indep/src/copmgr.hpp"
#include "game_core/indep/inc/World.hpp"
#include "game_core/indep/inc/Race.hpp"
#include "game_core/indep/src/trackgraph.hpp"
#include <cmath>
#include "game_core/port/race/car_reset_services.hpp"
#include "game_core/port/race/ai_drive_target.hpp"
#include "game_core/indep/src/raceparameters.hpp"
#include "game_core/indep/inc/Player.hpp"
#include "game_core/indep/src/timer.hpp"
#include "game_core/port/driving/projections.hpp"

namespace hp2::debug {
// Debug0021F7E8, E-0099-W50. First actual manager entry whose pMyCar matches;
// unlike GetCopByChase, this original loop does not skip null cop pointers.
std::uint32_t tCopManager::GetCopByCar(std::uint32_t car){for(int i=0;i<memory_.signed_word(address_+8);++i){const auto cop=memory_.word(address_+0x10+std::uint32_t(i)*4);if(memory_.word(cop+4)==car)return cop;}return 0;}
// Debug0021F840, E-0099-W50. Strict nearest actual pursuing cop; equal distance
// preserves earlier manager order. Original XYZ grouping and image threshold.
std::uint32_t tCopManager::GetCopByChase(std::uint32_t car){std::uint32_t chosen=0;float nearest=memory_.scalar(0x325fbc);for(int i=0;i<memory_.signed_word(address_+8);++i){const auto cop=memory_.word(address_+0x10+std::uint32_t(i)*4);if(!cop)continue;const auto pursuit=memory_.word(cop);if(!pursuit||memory_.word(pursuit+0x10)!=car)continue;const auto other=memory_.word(cop+4);const float x=memory_.scalar(car+0x60)-memory_.scalar(other+0x60),y=memory_.scalar(car+0x64)-memory_.scalar(other+0x64),z=memory_.scalar(car+0x68)-memory_.scalar(other+0x68);const float distance=std::sqrt((x*x+y*y)+z*z);if(distance<nearest){nearest=distance;chosen=cop;}}return chosen;}
}
#include "game_core/inc/bSlotPool.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/port/race/cop_resources.hpp"
#include "game_core/indep/src/aiutil.hpp"
#include "game_core/indep/src/trackroute.hpp"
#include "game_core/indep/src/trackpath.hpp"
#include "game_core/indep/src/track.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include <cstring>
#include <stdexcept>

namespace hp2::debug {
// Debug00220738, E-0099-W11. Real data is never replaced with an empty array.
void tCopManager::InitRoadBlockStructures(const ::port::race::CopConstructionServices& services){
    memory_.word(address_+0x13c,0);
    if(!services.resources||!services.resources->source_data_loaded)hp2::not_implemented("0x00220738","actual roadblock/radar source data has not been loaded");
    const auto data=memory_.word(0x00328c00);
    if(data&&memory_.word(0x00311f78+0x264)&&memory_.word(data)){
        const auto& resources=*services.resources;
        if(resources.owner!=&memory_||resources.roadblocks!=data||!resources.routes||!resources.routes->initialized||!resources.topology||!resources.topology->ready())
            hp2::not_implemented("0x00220738","original roadblock route/topology source owner is not bound");
        auto& memory=memory_;const auto& routes=*resources.routes;const bool forward=memory.signed_word(0x00311f78+0xc)==0;
        const unsigned count=memory.word(data);if(count>64)throw std::out_of_range("Original roadblock mapping capacity exceeded");
        unsigned accepted=0;
        for(unsigned i=0;i<count;++i){const auto source=data+4+i*28;memory.word(address_+0x1f40+i*4,0xffffffff);
            const auto bits=memory.word(source+24),direction=bits&3;
            if(!(direction==0||(direction==1&&forward)||(direction==2&&!forward)))continue;
            const std::array<float,2> centre{{memory.scalar(source+16),memory.scalar(source+20)}};
            NearestRoutePointInfo nearest;
            if(!GetNearestRoutePoint(memory,routes,nearest,centre,ALL_RACING_ROUTES,0,0))continue;
            const auto point=nearest.Point,row=address_+0x140+accepted*0x78;
            const float left=memory.scalar(point+0x18),right=memory.scalar(point+0x1c),width=left+right;
            memory.word(row+0x28,nearest.Route);memory.word(row+0x24,point);memory.scalar(row+0x2c,width);
            if(!(9.0f<=width))continue;
            if(!TopologyCoordinate::HasTopology(memory,*resources.topology,centre))continue;
            memory.word(row+0x20,source);memory.word(row+0x30,(bits>>2)&3);memory.word(row+0x68,i);
            memory.word(row+0x70,0);memory.word(row+0x6c,(bits>>4)&3);if(bits&0x40)memory.word(row+0x70,1);
            const auto dx=memory.scalar(point+0xc),dy=memory.scalar(point+0x10);
            const float d=(centre[0]-memory.scalar(point))*dx+(centre[1]-memory.scalar(point+4))*dy;
            memory.word(address_+0x1f40+i*4,accepted);
            const auto position=TrackRoutePoint(memory,point).GetPosition(d,(right-left)*0.5f);
            memory.scalar(row+0x44,position[0]);memory.scalar(row+0x48,position[1]);
            memory.scalar(row+0x4c,dx);memory.scalar(row+0x50,dy);memory.scalar(row+0x54,dy);memory.scalar(row+0x58,-dx);
            memory.half(row+0x1c,hp2::recovered::bATan(memory,dy,-dx));
            for(int cop=1;cop>=0;--cop)memory.word(row+unsigned(cop)*4,0);
            TrackPathCoordinate coordinate(routes.paths,position,memory.scalar(0x00325da0));memory.scalar(row+0x64,coordinate.DistanceAroundTrack);
            ++accepted;
        }
        memory.word(address_+0x13c,accepted);
    }
}
// Debug00221218, E-0099-W11. Original zero count does not clear mapping arrays.
void tCopManager::InitSpeedTrapStructures(const ::port::race::CopConstructionServices& services){
    memory_.word(address_+0x2040,0);
    if(!services.resources||!services.resources->source_data_loaded)hp2::not_implemented("0x00221218","actual radar-trap source data has not been loaded");
    const auto data=memory_.word(0x00328bfc);
    if(data&&memory_.word(data)){
        const auto& resources=*services.resources;
        if(resources.owner!=&memory_||resources.radar_traps!=data||!resources.routes||!resources.routes->initialized||!resources.topology||!resources.topology->ready())
            hp2::not_implemented("0x00221218","original radar-trap route/topology source owner is not bound");
        auto& memory=memory_;const auto& routes=*resources.routes;
        const auto count=memory.word(data);if(count>64)throw std::out_of_range("Original radar-trap mapping capacity exceeded");
        for(unsigned i=0;i<count;++i){const auto source=data+4+i*32;const std::array<float,2> position{{memory.scalar(source+16),memory.scalar(source+20)}};
            memory.word(address_+0x2744+i*4,0xffffffff);
            if(!TopologyCoordinate::HasTopology(memory,*resources.topology,position))continue;
            TrackPathCoordinate coordinate(routes.paths,position,memory.scalar(0x00325da0));
            const auto index=memory.word(address_+0x2040),row=address_+0x2044+index*28;
            if(index>=64)throw std::out_of_range("Original radar-trap storage capacity exceeded");
            memory.scalar(row+0x14,coordinate.DistanceAroundTrack);memory.word(row+0x18,0);
            memory.scalar(row+8,position[1]);memory.word(row,0);memory.scalar(row+4,position[0]);
            if(memory.word(source+24))memory.word(row+0x18,memory.word(row+0x18)|1);
            else if(memory.word(source+28))memory.word(row+0x18,memory.word(row+0x18)|2);
            NearestRoutePointInfo nearest;
            if(!GetNearestRoutePoint(memory,routes,nearest,position,ALL_RACING_ROUTES,0,0))
                hp2::not_implemented("0x00221218","original radar-trap nearest route was invalid");
            const auto angle=hp2::recovered::bATan(memory,memory.scalar(nearest.Point+0xc),memory.scalar(nearest.Point+0x10));
            memory.word(address_+0x2744+i*4,index);memory.word(address_+0x2040,index+1);memory.word(row+0x10,source);memory.half(row+0xc,angle);
        }
    }
}
// Debug0021F1E0, E-0099-W11. Preserve range branches and the World-owned RNG;
// eight constructor calls intentionally consume eight independent samples.
float tCopManager::GetBaseSpeedTrapSpawnTime(){
    auto& memory=memory_;
    const auto parameters=0x00311f78;
    const float laps=memory.word(parameters+0x28)?1.0f:static_cast<float>(memory.signed_word(parameters+0x2c));
    const float distance=laps*memory.scalar(0x00313308);
    float range=45;
    if(distance<memory.scalar(0x00325fb4))range=15;
    else if(distance<memory.scalar(0x00325fb8))range=30;
    if(memory.word(parameters+0x18))return 1;
    if(memory.word(0x00326e9c))return memory.scalar(0x00326eb0);
    const auto world=memory.word(0x003272f0);
    if(!world)hp2::not_implemented("0x0014E230","World-owned police spawn RNG is not bound");
    return range+hp2::recovered::bRandom(memory,world+0x178,range);
}
// Debug0021FE30, E-0099-W11. Empty deque returns an undefined incoming register
// in the original; the port fails rather than inventing a speaker identifier.
std::uint32_t tCopManager::ReserveCopSpeaker(){
    const auto deque=address_+0x2844;
    auto head=memory_.word(deque+0x10);
    if(memory_.word(deque+0x18)==0&&head==memory_.word(deque+0x14))
        hp2::not_implemented("0x0021FE30","empty original CopSpeakers deque has no defined return value");
    if(head>=4)hp2::not_implemented("0x0021FE30","CopSpeakers head outside original capacity");
    const auto value=memory_.word(deque+4*head);
    head=head+1;if(head==4)head=0;
    memory_.word(deque+0x10,head);memory_.word(deque+0x18,0);return value;
}
// Debug00223368, E-0099-W11. The original function is an empty return.
void tCopManager::InitRoadClosures(){}
// Debug0021DE80, E-0099-W11. Original arrays with trivial constructors remain
// untouched, including pullover display flags and roadblock/radar map tails.
void tCopManager::Sub_0021DE80(const ::port::race::CopConstructionServices& services){
    auto& memory=memory_;const auto manager=address_;
    for(unsigned offset:{0x2cu,0x50u}){memory.word(manager+offset,manager+offset);memory.word(manager+offset+4,manager+offset);}
    memory.word(manager+0x2910,0x002b4de8);
    const auto deque=manager+0x2844;
    memory.word(deque+0x10,0);memory.word(deque+0x14,0);memory.word(deque+0x18,0);
    memory.word(0x00328c04,bNewSlotPool(memory,0x640,6,0x002b4be0,0x10));
    memory.word(0x00328c08,bNewSlotPool(memory,0x5c,6,0x002b4bf0,0x10));
    memory.word(0x00328c0c,bNewSlotPool(memory,0x900,10,0x002b4c00,0x10));
    for(int i=5;i>=0;--i)memory.word(manager+0x34+4*static_cast<unsigned>(i),0);
    memory.word(manager+0x4c,0);memory.word(manager+0x138,0);
    InitRoadBlockStructures(services);InitSpeedTrapStructures(services);
    memory.word(manager+4,0);memory.word(manager+0xc,0xffffffff);memory.word(manager+8,0);
    for(unsigned i=0;i<7;++i)memory.word(manager+0x10+4*i,0);
    for(unsigned i=0;i<4;++i){
        if(!memory.word(deque+0x18)){
            const auto old=memory.word(deque+0x10);const auto head=old?old-1:3;
            memory.word(deque+0x10,head);memory.word(deque+head*4,i);
            memory.word(deque+0x18,memory.word(deque+0x14)==head);
        }
    }
    memory.word(manager+0x2864,0);memory.scalar(manager+0x286c,-50);memory.scalar(manager+0x2868,12);
    if(!services.allocate_voices)hp2::not_implemented("0x00205158","original police speech voice allocation/RNG is not bound");
    services.allocate_voices(memory,memory.word(0x00328c18));
    if(!services.flush_speech_queue)hp2::not_implemented("0x00205100","original speech queue/noise flush is not bound");
    services.flush_speech_queue(memory,memory.word(0x00328c18));
    for(unsigned i=0;i<8;++i){
        const auto pullover=manager+0x58+i*0x1c;
        memory.word(pullover+0xc,0);memory.word(manager+0x2874+i*4,0);
        memory.word(pullover+0x10,0);memory.word(pullover+0x14,0);
        memory.word(pullover+4,0);memory.byte(pullover+8,0);memory.word(pullover,0xffffffff);
    }
    for(unsigned i=0;i<8;++i)memory.scalar(manager+0x2894+i*4,GetBaseSpeedTrapSpawnTime());
    memory.word(manager+0x2870,0);
    for(unsigned i=0;i<14;++i)memory.word(manager+0x28b4+i*4,0);
    const auto parameters=0x00311f78;
    if(memory.word(parameters+0x274)){
        memory.word(manager+0x2860,ReserveCopSpeaker());
        const auto scale=memory.scalar(0x00325fb0);
        memory.word(manager,memory.word(parameters+0x280));
        memory.word(manager+0x28fc,memory.word(parameters+0x284));
        memory.scalar(manager+0x28f0,static_cast<float>(memory.signed_word(parameters+0x24))*scale);
    }else{memory.word(manager+0x28f0,0);memory.word(manager,0);memory.word(manager+0x28fc,0);}
    const auto strength=memory.signed_word(parameters+0x288);
    memory.word(manager+0x28ec,strength==0?0:strength==1?1:2);
    memory.word(manager+0x28f4,0);memory.word(manager+0x28f8,0);memory.word(manager+0x2900,0);
    InitRoadClosures();memory.word(manager+0x2904,0);memory.word(manager+0x2908,0);memory.word(manager+0x290c,0);
}
// Debug00222490, E-0099-W11. Disabled original branch preserves existing global.
void InitCops(::port::driving::Memory& memory,const ::port::race::CopConstructionServices& services){
    if(memory.signed_word(0x00311f78+0x25c)>0){
        const auto manager=memory.allocate(0x2914);
        tCopManager object(memory,manager);object.Sub_0021DE80(services);
        memory.word(0x00328c1c,manager);
    }
}
// Debug0021E270, E-0099-W17. Required active-gameplay destructors fail at their
// original calls. The real constructor creates empty lists/null cop pointers,
// for which the original list destructors have no remaining stores or calls.
void tCopManager::Sub_0021E270(int in_chrg){
    auto& memory=memory_;memory.word(address_+0x2910,0x002b4de8);
    if(const auto dummy=memory.word(address_+0x4c)){DeletePursuit(dummy);memory.word(address_+0x4c,0);} //W119.
    while(memory.word(address_+0x2c)!=address_+0x2c)DeletePursuit(memory.word(address_+0x2c));
    for(unsigned i=0;i<7;++i)if(const auto cop=memory.word(address_+0x10+i*4)){
        const auto table=memory.word(cop+0x1c);const auto object=cop+static_cast<std::uint32_t>(static_cast<std::int32_t>(memory.signed_half(table+8)));
        switch(memory.word(table+0xc)){case 0x228338:tAICop(memory,object).Sub_00228338(3);break;case 0x227c38:tPlayerCop(memory,object).Sub_00227C38(3);break;case 0x227aa0:tCop(memory,object).Sub_00227AA0(3);break;default:hp2::not_implemented("0x0021E310","unrecovered original tCop destructor target");}
    }
    if(memory.word(address_+0x50)!=address_+0x50)
        hp2::not_implemented("0x0021E358","active original tSpikeStrip virtual destruction is not implemented");
    bDeleteSlotPool(memory,memory.word(0x00328c04));memory.word(0x00328c04,0);
    bDeleteSlotPool(memory,memory.word(0x00328c08));memory.word(0x00328c08,0);
    bDeleteSlotPool(memory,memory.word(0x00328c0c));memory.word(0x00328c0c,0);
    if(in_chrg&1)memory.release(address_);
}
// Debug002224D8, E-0099-W17. Publish null only after destructor succeeds.
void CleanupCops(::port::driving::Memory& memory){
    if(const auto manager=memory.word(0x00328c1c)){
        tCopManager(memory,manager).Sub_0021E270(3);memory.word(0x00328c1c,0);
    }
}
}

namespace port::race {
// Debug002225F8, E-0099-W17. Preserve complete original chunk payloads and
// source ordering. Logical allocation/lifetime is port-owned; metadata is not
// replaced with generated positions, synthetic routes or empty scenery.
CopResources load_cop_resources(::port::driving::Memory& memory,const std::vector<std::uint8_t>& bytes,
    const ::port::track::RouteData& routes,const ::port::track::Topology& topology){
    if(!routes.initialized||!topology.ready())throw std::logic_error("Cop resources require actual initialized routes and topology");
    if(memory.word(0x00328c00)||memory.word(0x00328bfc)||memory.word(0x00328bf8))
        throw std::logic_error("Cop source globals already have an owner");
    CopResources result;result.owner=&memory;result.routes=&routes;result.topology=&topology;
    const auto word=[&](std::size_t at){if(at>bytes.size()||bytes.size()-at<4)throw std::out_of_range("Truncated original cop source word");
        return std::uint32_t(bytes[at])|std::uint32_t(bytes[at+1])<<8|std::uint32_t(bytes[at+2])<<16|std::uint32_t(bytes[at+3])<<24;};
    std::size_t consumed=0;const auto chunks=hp2::assets::parse_chunks_top(bytes,&consumed);
    if(consumed!=bytes.size())throw std::out_of_range("Incomplete original cop source chunk stream");
    const auto visit=[&](const auto& self,const std::vector<hp2::assets::Chunk>& siblings)->void{
        for(const auto& container:siblings){
            if(container.tag==0x80034500){
                for(const auto& chunk:container.children){
                    std::uint32_t global=0,stride=0;std::uint32_t* destination=nullptr;
                    if(chunk.tag==0x34510){global=0x00328bfc;stride=32;destination=&result.radar_traps;}
                    else if(chunk.tag==0x34520){global=0x00328c00;stride=28;destination=&result.roadblocks;}
                    else if(chunk.tag==0x34530){global=0x00328bf8;stride=60;destination=&result.landmarks;}
                    else continue;
                    if(chunk.size<4)throw std::out_of_range("Truncated original cop data header");
                    const auto count=word(chunk.payload_offset());
                    if(std::uint64_t(count)*stride+4>chunk.size)throw std::out_of_range("Original cop record count exceeds source chunk");
                    if(chunk.tag!=0x34530&&count>64)throw std::out_of_range("Original cop data exceeds64 mapping slots");
                    const auto address=memory.allocate(chunk.size);
                    std::memcpy(memory.view(address,chunk.size).data,bytes.data()+chunk.payload_offset(),chunk.size);
                    result.owned_allocations.push_back(address);*destination=address;memory.word(global,address);
                }
            }else self(self,container.children);
        }
    };visit(visit,chunks);result.source_data_loaded=true;return result;
}
// Debug002226B0 plus port storage lifetime, E-0099-W17. Caller must first
// destroy every manager that holds Name/source pointers into these records.
void unload_cop_resources(::port::driving::Memory& memory,CopResources& resources){
    if(resources.owner!=&memory)throw std::logic_error("Cop source owner belongs to another Memory");
    if(resources.radar_traps)memory.word(0x00328bfc,0);
    if(resources.roadblocks)memory.word(0x00328c00,0);
    if(resources.landmarks)memory.word(0x00328bf8,0);
    for(auto allocation:resources.owned_allocations)memory.release(allocation);
    resources={};
}
}

#include "game_core/indep/inc/CopChase.hpp"
namespace hp2::debug {
// Debug00222BA8, E-0099-W38. Exact source normal/player-cop branches and raw
// shared manager arrays. The eligible AI DisableMyCar child remains required.
void tCopManager::MonitorImpact(std::uint32_t car,float magnitude,bool two_car){
 auto& m=memory_;const auto difficulty=[&](){const int level=m.signed_word(address_+0x28ec);if(level<0||level>2)hp2::not_implemented("0x00222BA8","Original cop difficulty outside authored3-entry tables");return static_cast<unsigned>(level);};
 if(!m.word(0x311f78+0x274)){
  if(m.word(car+0x20)!=4)return;if(two_car)magnitude*=1.5f;const auto level=difficulty();if(!(m.scalar(0x313ba8+level*4)<=magnitude))return;
  const auto cop=GetCopByCar(car);if(!cop)return;const auto vtable=m.word(cop+0x1c),target=m.word(vtable+0x14);const auto receiver=cop+static_cast<std::int32_t>(m.signed_half(vtable+0x10));bool controlled;
  if(target==0x22b380)controlled=tAICop(m,receiver).IsAIControlled();else if(target==0x22b3b8)controlled=tPlayerCop(m,receiver).IsAIControlled();else hp2::not_implemented("0x00222D7C","Original cop IsAIControlled virtual target is required");
  if(controlled)hp2::not_implemented("0x0022B138","Original tAICop.DisableMyCar damage/pursuit/speech/state chain is required");return;
 }
 const int index=m.signed_word(address_+12);if(index<0||index>=7)hp2::not_implemented("0x00222BC8","Original player cop index outside manager table");const auto cop=m.word(address_+16+static_cast<unsigned>(index)*4);if(!cop)return;
 const auto pursuit=m.word(cop);if(!pursuit)return;const auto own=m.word(cop+4),chased=m.word(pursuit+0x10);if(chased!=car&&own!=car)return;
 if(m.word(0x311f78+0x30)!=2){const float dy=m.scalar(own+0x64)-m.scalar(chased+0x64),dx=m.scalar(own+0x60)-m.scalar(chased+0x60);if(!(std::sqrt(dx*dx+dy*dy)<50))return;}
 if(!(5<=magnitude))return;const int car_index=m.signed_half(car+8);if(car_index<0||car_index>=14)hp2::not_implemented("0x00222C6C","Original CarIndex outside14 cop-damage entries");const auto value=address_+0x28b4+static_cast<unsigned>(car_index)*4,level=difficulty();
 if(m.scalar(0x313ba8+level*4)<=magnitude){m.scalar(value,1);return;}
 const float factor=m.scalar(0x2b4db0+level*4)*m.scalar(0x325fe0);float delta=magnitude*factor;if(delta>1)delta=1;float damage=m.scalar(value)+delta;if(damage>1)damage=1;m.scalar(value,damage);
}
}

namespace hp2::debug {
// Debug00221C78, E-0099-W53. Actual intrusive PursuitList first-match query;
// sentinel-only list returns null, with no synthesized pursuit object.
std::uint32_t tCopManager::IsBeingPursued(std::uint32_t car){const auto head=address_+0x2c;for(auto pursuit=memory_.word(head);pursuit!=head;pursuit=memory_.word(pursuit))if(memory_.word(pursuit+0x10)==car)return pursuit;return 0;}
}

namespace hp2::debug {
// Debug002211D8/00220AA8; E-0099-W47. Active roadblocks are non-null
// pursuit-owned pointers, not the separately loaded roadblock geometry table.
int tCopManager::GetNumActiveRoadBlocks()const{const auto head=address_+0x2c;int count=0;for(auto n=memory_.word(head);n!=head;n=memory_.word(n))if(memory_.word(n+0x14))++count;return count;}
std::uint32_t tCopManager::GetActiveRoadBlock(int index)const{const auto head=address_+0x2c;int count=0;for(auto n=memory_.word(head);n!=head;n=memory_.word(n))if(const auto roadblock=memory_.word(n+0x14)){if(count==index)return roadblock;++count;}return 0;}
}

namespace port::race {
// Port-only shared projection of the source inline RC predicate. No RC owner
// is fabricated here: the original dereference requires an actual bound owner.
static bool cop_frame_replay_mode(::port::driving::Memory& m,const char* site){
 const auto coordinator=m.word(0x327290);if(!coordinator)hp2::not_implemented(site,"original police frame requires RaceCoordinator owner");
 const auto state=m.word(coordinator);return state==4||state==7||state==5||state==9;
}
}
namespace hp2::debug {
// Debug00220058, E-0099-W61. Actual player-owned first pursuit, or the
// player-cop's borrowed pursuit in BeACop. No synthetic pursuit state.
std::uint32_t tCopManager::FindPlayerPursuit(){auto& m=memory_;if(!m.word(0x3121ec)){const auto head=address_+0x2c;for(auto p=m.word(head);p!=head;p=m.word(p))if(m.word(m.word(p+0x10)+0x18))return p;return 0;}const int index=m.signed_word(address_+0xc);if(index==-1)return 0;if(index<0||index>=7)hp2::not_implemented("0x00220058","original player-cop index exceeds seven entries");const auto cop=m.word(address_+0x10+4*std::uint32_t(index));return cop?m.word(cop):0;}
// Debug0021FED8, E-0099-W61. Replay gating precedes every speech-manager
// access. The timer update is committed before testing its strict upper bound.
void tCopManager::TimeBasedSpeech(){auto& m=memory_;if(::port::race::cop_frame_replay_mode(m,"0x0021FED8"))return;const auto speech=m.word(0x328c18);if(!speech)hp2::not_implemented("0x0021FF34","original CopSpeechMgr owner is required");
 if(!m.word(speech+0x5ac)&&m.word(address_+0x2c)!=address_+0x2c){const float elapsed=m.scalar(address_+0x2864)+m.scalar(0x327d08);m.scalar(address_+0x2864,elapsed);if(!(m.scalar(address_+0x2868)<elapsed))return;const auto pursuit=FindPlayerPursuit();if(pursuit&&!(m.scalar(address_+0x286c)>0))hp2::not_implemented("0x00226168","original pursuit AboutToSaySomething/NotInChasingState and filler speech remain required");}
 m.word(address_+0x2864,0);
}
// Debug0021FB80, E-0099-W61. Decrement and zero publication precede actual
// pursuit activation calls. Empty original lists genuinely complete this pass.
void tCopManager::SupervisorActivate(){auto& m=memory_;if(::port::race::cop_frame_replay_mode(m,"0x0021FB80"))return;const float timer=m.scalar(address_+0x286c);if(!(timer>0))return;const float updated=timer-m.scalar(0x327d08);m.scalar(address_+0x286c,updated);if(!(updated<=0))return;m.word(address_+0x286c,0);const auto head=address_+0x2c;for(auto pursuit=m.word(head);pursuit!=head;pursuit=m.word(pursuit))if(m.word(m.word(pursuit+0x10)+0x18))hp2::not_implemented("0x002252D8","original pursuit CanSupervisorInitiate/SuperInitiates chain is required");}
// Debug00220440, E-0099-W61. The first nonempty original pursuit reaches its
// real Update child; no active pursuit is silently retained or discarded.
void tCopManager::PursuitManagement(::port::race::PhysicsConstructionServices* physics){auto& m=memory_;const auto head=address_+0x2c;for(auto pursuit=m.word(head);pursuit!=head;){const auto next=m.word(pursuit);if(tPursuit(m,pursuit).Update(physics&&physics->disable_pursuit_speech_emission,physics?physics->pursuit_add_point_w:std::nullopt,physics?physics->pursuit_set_point_w:std::nullopt,physics))DeletePursuit(pursuit);pursuit=next;}}
// Debug0021F0C0,E-0110-P01; source ordered first ready LIMBO AI cop.
std::uint32_t tCopManager::FindCopForSpeedTrap(){auto& m=memory_;for(int i=0;i<m.signed_word(address_+4);++i){if(i>=7)hp2::not_implemented("0x0021F0C0","Original AI cop table exceeds seven entries");const auto cop=m.word(address_+0x10+4*std::uint32_t(i));if(m.word(cop+0x10)==1&&m.word(cop+0x14)==12)return cop;}return 0;}
// Debug00221420,E-0110-P01. Original loaded-data mapping, not a direct cast.
std::uint32_t tCopManager::GetSpeedTrapFromLoadedDataIndex(int index){if(std::uint32_t(index)>=64)throw std::out_of_range("Original speed-trap loaded index exceeds mapping carrier");const auto mapped=memory_.signed_word(address_+0x2744+4*std::uint32_t(index));if(mapped==-1)return 0;if(std::uint32_t(mapped)>=64)throw std::out_of_range("Original speed-trap mapped index exceeds storage carrier");return address_+0x2044+28*std::uint32_t(mapped);}
// Debug0021F2A8,E-0110-P01. Original pursuit count, split-time/random admission,
// progress/ticket test, graph search, actual placement and one-spawn return.
void tCopManager::SpawnSpeedTraps(::port::race::PhysicsConstructionServices* physics,const ::port::race::CarResetServices* reset){
 auto& m=memory_;const auto head=address_+0x2c;int pursuits=0;for(auto p=m.word(head);p!=head;p=m.word(p))++pursuits;
 const auto race=m.word(0x327258);if(!race)hp2::not_implemented("0x0021F2A8","Original speed-trap scheduling requires Race owner");
 for(int i=0;i<m.signed_word(race+0x18);++i){
  const auto racing=m.word(race+0x624+4*std::uint32_t(i));if(!racing)continue;
  const auto car=m.word(racing);const int index=m.signed_half(car+8);if(index<0||index>=8)hp2::not_implemented("0x0021F2A8","Original racer CarIndex exceeds eight timer entries");
  if(m.signed_half(racing+0x12)||!(m.scalar(address_+0x2874+4*std::uint32_t(index))<=0))continue;
  const auto timer=address_+0x2894+4*std::uint32_t(index);const float before=m.scalar(timer);
  if(before>0){m.scalar(timer,before-m.scalar(0x327d08));continue;}
  const int difficulty=m.signed_word(address_+0x28ec);if(difficulty<0||difficulty>2)hp2::not_implemented("0x0021F2A8","Original difficulty outside three speed-trap tables");
  const auto world=m.word(0x3272f0);if(!world)hp2::not_implemented("0x0021F2A8","Original speed-trap probability requires World seed");
  if(World(m,world).Random(m.scalar(0x2b4ce0+4*std::uint32_t(difficulty)))>1){m.scalar(timer,m.scalar(0x2b4cd0+4*std::uint32_t(difficulty)));continue;}
  bool eligible=false;const auto pursuit=IsBeingPursued(car);
  if(pursuit){const int cops=m.signed_word(pursuit+0x5c4);eligible=cops==0||(cops<m.signed_word(pursuit+8)&&m.word(pursuit+0x5ec)!=0);}
  else if(pursuits<2||m.word(car+0x18)){
   const auto leader=m.signed_word(race+0x18)>0?m.word(race+0x624):0;Timer split;Race(m,race).CalculateSplitTime(split,leader,racing);
   const auto threshold=::port::driving::cvt_w_s(-15.f*4000.f+.5f); // Original Timer.SetTime00136D80.
   bool admitted=threshold<split.PackedTime;
   if(!admitted)admitted=m.signed_half(leader+0x12)!=0||World(m,world).Random(20)==19;
   if(!admitted)m.scalar(timer,1);
   else if(RaceParameters(m,0x311f78).IsFreeRun()||m.word(car+0x18))eligible=true;
   else{
    const float laps=m.word(0x311f78+0x28)?1.f:float(m.signed_word(0x311f78+0x2c));
    const auto tickets=m.signed_byte(address_+0x60+28*std::uint32_t(index));const float maximum=float(static_cast<std::int32_t>(m.word(0x311f78+0x27c)+1));
    if(laps==0||maximum==0)hp2::not_implemented("0x0021F55C","Exceptional original progress/ticket division requires its EE numerical carrier");
    eligible=float(tickets)/maximum<m.scalar(racing+0x20)/laps;
   }
  }
  if(!eligible)continue;
  const auto cop=FindCopForSpeedTrap();if(!cop)continue;
  const auto graph=m.word(0x328ef8);if(!graph)hp2::not_implemented("0x00247F00","Original speed-trap search requires its shared TrackGraph owner");
  ::port::race::AiDriveTargetServices ai;ai.routes=reset?reset->routes:nullptr;
  const auto trap=tTrackGraph(m,graph).FindSpeedTrap(car,250.f,m.scalar(0x313308)*.5f,true,ai);
  if(!trap||m.word(trap))continue;
  if(!IsTeleportLocationClear(m,{{m.scalar(trap+4),m.scalar(trap+8)}}))continue;
  tAICop(m,cop).SetSpeedTrap(trap,physics,reset);m.scalar(timer,20);return;
 }
}
// Debug0021F688, E-0099-W61. Actual patrol distance and original inline
// SetDesiredLogicState00227BC0 store, independently corroborated by listing.
void tCopManager::ReclaimPatrollers(){auto& m=memory_;for(int i=0;i<m.signed_word(address_+4);++i){if(i>=7)hp2::not_implemented("0x0021F688","original AI cop table exceeds seven entries");const auto cop=m.word(address_+0x10+4*std::uint32_t(i));if(m.word(cop+0x10)!=5)continue;const auto world=m.word(0x3272f0);if(!world)hp2::not_implemented("0x0021F688","original patrol distance requires World owner");const auto car=m.word(cop+4);if(World(m,world).GetDistanceToClosestPlayer(std::array<float,2>{{m.scalar(car+0x60),m.scalar(car+0x64)}})>=250)m.word(cop+0x10,1);}}
// Debug00221F98, E-0099-W61. Source reads World before its actual empty-list
// return. Positive strip Update/intersection/blowout remains a reached callee.
void tCopManager::MonitorSpikeBelts(){const auto world=memory_.word(0x3272f0);if(!world)hp2::not_implemented("0x00221F98","original spike monitor requires World owner");(void)memory_.word(world+0x1c);if(memory_.word(address_+0x50)!=address_+0x50)hp2::not_implemented("0x00221F98","original active spike Update/intersection/tire-blowout pipeline is required");}
// Debug002216E0, E-0099-W61. Fullword bool reads and strict negative expiry;
// zero is retained until a later source frame, with no inferred clamp.
void tCopManager::MonitorPullovers(const ::port::race::CarResetServices* reset){auto& m=memory_;for(unsigned i=0;i<8;++i){const auto entry=address_+0x58+0x1c*i;if(m.word(entry+0xc)&&!m.word(entry+0x10)){const float remaining=m.scalar(entry+4)-m.scalar(0x327d08);m.scalar(entry+4,remaining);if(remaining<0)EndPullover(static_cast<int>(i),reset);}}}
// Debug00222480, E-0099-W61. Only stack adjust/JR/restore in original listing.
void tCopManager::RenderDebugInfo(){}
// Debug0021ED20, E-0099-W61. Original JR RA/NOP.
void tCopManager::LookForDanglyPointers(int){}
// Debug0021EF58, E-0099-W61. Original complete outer call order. This is a
// scoped frame projection: active child pipelines still throw where reached.
void tCopManager::Think(::port::race::PhysicsConstructionServices* physics,const ::port::race::CarResetServices* reset){auto& m=memory_;TimeBasedSpeech();SupervisorActivate();PursuitManagement(physics);if(!m.word(0x3121ec)){SpawnSpeedTraps(physics,reset);const auto race=m.word(0x327258);if(!race)hp2::not_implemented("0x0021EF58","original cop frame requires Race owner");if(m.signed_word(race+0x10)>0)for(unsigned i=0;i<8;++i){const auto entry=address_+0x2874+4*i;const float before=m.scalar(entry);if(before>0){const float after=before-m.scalar(0x327d08);m.scalar(entry,after<0?0:after);}}}else ReclaimPatrollers();MonitorSpikeBelts();MonitorPullovers(reset);for(int i=0;i<m.signed_word(address_+8);++i){if(i>=7)hp2::not_implemented("0x0021EF58","original Cop table exceeds seven owners");const auto cop=m.word(address_+0x10+4*std::uint32_t(i));if(!cop)hp2::not_implemented("0x0021EF58","actual cop owner absent before source virtual Think");const auto table=m.word(cop+0x1c),self=cop+static_cast<std::uint32_t>(static_cast<std::int32_t>(m.signed_half(table+0x18)));switch(m.word(table+0x1c)){case 0x2285f8:tAICop(m,self).Think(physics);break;case 0x227c50:hp2::not_implemented("0x00227C50","actual tPlayerCop Think/control/pursuit chain required");default:hp2::not_implemented("0x0021EF58","unrecovered actual cop virtual Think target");}}if(m.word(0x3121ec))hp2::not_implemented("0x00222768","original BeACopThink gameplay pipeline remains required");RenderDebugInfo();LookForDanglyPointers(1);}
// Debug002232E0, E-0099-W61. Original first-containing polygon among raw
// retained60-byte records. Retail00228A58 independently confirms these fields.
std::uint32_t FindLandmark(::port::driving::Memory& m,const std::array<float,2>& position){const auto data=m.word(0x328bf8);if(!data)hp2::not_implemented("0x002232E0","actual loaded LandmarkData source is required");for(std::uint32_t i=0;i<m.word(data);++i){const auto row=data+4+0x3c*i;if(IsPointInPoly(m,position,row+4,m.signed_word(row)))return row;}return 0;}
// Debug00222530, E-0099-W61. Manager scheduling precedes original graph
// fullword cloak check; source first-matching landmark and graph mutation kept.
void GlobalCopThink(::port::driving::Memory& m,float,::port::race::PhysicsConstructionServices* physics,const ::port::race::CarResetServices* reset){if(const auto manager=m.word(0x328c1c))tCopManager(m,manager).Think(physics,reset);const auto graph=m.word(0x328ef8);if(!graph)hp2::not_implemented("0x00222530","original GlobalCopThink requires TrackGraph owner");if(m.word(graph+8)){const auto world=m.word(0x3272f0);if(!world)hp2::not_implemented("0x00222530","original graph uncloaking requires World owner");for(int i=0;i<m.signed_word(world+0x24);++i){if(i>=14)hp2::not_implemented("0x00222530","original World table exceeds14 entries");const auto car=m.word(world+0x2c+4*std::uint32_t(i));const auto landmark=FindLandmark(m,{{m.scalar(car+0x60),m.scalar(car+0x64)}});if(landmark&&m.word(landmark+0x38)){tTrackGraph(m,graph).UncloakAllEdges();return;}}}}
}

// TASK-0099 E-0099-W76. Debug0021E3F8/00222510; original copmgr.cpp.
// Logical addresses and temporary storage are port adapters, not host PS2 ABI.
#include "game_core/indep/inc/Replay.hpp"
#include "game_core/indep/src/pursuit.hpp"
namespace hp2::debug {
void tCopManager::DoSnapshot(std::uint32_t snapshot){
 auto& m=memory_;ReplaySnapshot snap(m,snapshot);const auto a=address_;
 snap.BeginSection(a,0x2b4c18);
 snap.VerifyData(a+4,4,4,1);snap.VerifyData(a+0x13c,4,4,1);snap.VerifyData(a+0x2040,4,4,1);
 const bool restoring=m.byte(snapshot+0x3d)!=0;
 if(restoring){
  for(int i=0;i<m.signed_word(a+0x13c);++i){m.word(a+0x140+0x78*std::uint32_t(i),0);m.word(a+0x144+0x78*std::uint32_t(i),0);}
  for(int i=0;i<m.signed_word(a+0x2040);++i)m.word(a+0x2044+0x1c*std::uint32_t(i),0);
 }
 // Original stack count is uint8, independent of full list length.
 ::port::driving::Memory::Temporary storage(m,16);const auto tmp=storage.address();
 const auto pursuits=a+0x2c,spikes=a+0x50;
 unsigned count=0;for(auto p=m.word(pursuits);p!=pursuits;p=m.word(p))++count;
 m.byte(tmp,static_cast<std::uint8_t>(count));snap.MoveData(tmp,1,1,0);
 if(restoring){
  while(m.word(pursuits)!=pursuits)DeletePursuit(m.word(pursuits));
  for(unsigned i=0;i<m.byte(tmp);++i){snap.MoveData(tmp+1,1,1,1);CreatePursuit(m.signed_byte(tmp+1),0);}
 }else for(auto p=m.word(pursuits);p!=pursuits;p=m.word(p)){m.byte(tmp+2,m.byte(p+0x5c8));snap.MoveData(tmp+2,1,1,1);}
 for(auto p=m.word(pursuits);p!=pursuits;p=m.word(p))tPursuit(m,p).DoSnapshot(snapshot); //W119.
 snap.VerifyMoveDataInSync(0x2b4c18,0x2b4c28);
 for(int i=0;i<m.signed_word(a+8);++i){const auto cop=m.word(a+0x10+4*std::uint32_t(i)),table=m.word(cop+0x1c),target=m.word(table+0x24),adjusted=cop+std::uint32_t(std::int32_t(m.signed_half(table+0x20)));if(target==0x228398)tAICop(m,adjusted).DoSnapshot(snapshot);else hp2::not_implemented("0x0021E6F4","actual cop virtual DoSnapshot child is required");}
 snap.VerifyMoveDataInSync(0x2b4c18,0x2b4c38);
 count=0;for(auto p=m.word(spikes);p!=spikes;p=m.word(p))++count;
 m.word(tmp+4,count);snap.MoveData(tmp+4,4,4,1);
 if(restoring){
  if(const auto strip=m.word(spikes);strip!=spikes){const auto next=m.word(strip),prev=m.word(strip+4),table=m.word(strip+0x8f0);m.word(prev,next);m.word(next+4,prev);(void)m.word(table+0xc);(void)m.signed_half(table+8);hp2::not_implemented("0x0021E77C","actual spike virtual destructor flags3 after original unlink is required");}
  if(m.signed_word(tmp+4)>0)hp2::not_implemented("0x00251050","actual SpikeStrip pool allocation and constructor0023AEC0 during restore are required");
 }
 if(m.word(spikes)!=spikes)hp2::not_implemented("0x0023B450","actual tSpikeStrip.DoSnapshot child is required");
 snap.VerifyMoveDataInSync(0x2b4c18,0x2b4c48);
 for(unsigned i=0;i<8;++i){const auto entry=a+0x58+0x1c*i;snap.MoveData(entry+0xc,1,4,0);snap.MoveData(entry+0x10,1,4,0);snap.MoveData(entry+0x14,1,4,0);snap.MoveData(entry+4,4);snap.MoveData(entry+8,1,1,1);snap.MoveData(entry,4,4,1);}
 snap.MoveData(a+0x138,1,4,0);
 snap.VerifyMoveDataInSync(0x2b4c18,0x2b4c60);
 const auto deque=a+0x2844;
 if(!restoring){
  const bool empty=!m.word(deque+0x18)&&m.word(deque+0x14)==m.word(deque+0x10);
  if(!empty){const auto start=m.word(deque+4*m.word(deque+0x10));do{
   // Original pop-tail then push-head rotates the real deque before MoveData.
   if(m.word(deque+0x18)||m.word(deque+0x14)!=m.word(deque+0x10)){const auto tail=m.word(deque+0x14);m.word(deque+0x14,tail?tail-1:3);m.word(deque+0x18,0);m.word(tmp+8,m.word(deque+4*m.word(deque+0x14)));}
   if(!m.word(deque+0x18)){const auto head=m.word(deque+0x10);m.word(deque+0x10,head?head-1:3);const auto h=m.word(deque+0x10);m.word(deque+4*h,m.word(tmp+8));m.word(deque+0x18,m.word(deque+0x14)==h);}
   snap.MoveData(tmp+8,4,4,0);if(m.word(tmp+8)==start)break;
  }while(true);}
  m.word(tmp+8,6);snap.MoveData(tmp+8,4,4,0);
 }else{
  m.word(deque+0x18,0);m.word(deque+0x14,m.word(deque+0x10));
  for(;;){snap.MoveData(tmp+8,4,4,0);if(m.word(tmp+8)==6)break;if(!m.word(deque+0x18)){const auto head=m.word(deque+0x10);m.word(deque+0x10,head?head-1:3);const auto h=m.word(deque+0x10);m.word(deque+4*h,m.word(tmp+8));m.word(deque+0x18,m.word(deque+0x14)==h);}}
 }
 snap.VerifyMoveDataInSync(0x2b4c18,0x2b4c78);
 const auto world=m.word(0x3272f0);
 for(int i=0;i<m.signed_word(world+0x24);++i){snap.MoveData(a+0x2874+4*std::uint32_t(i),4);snap.MoveData(a+0x2894+4*std::uint32_t(i),4);}
 snap.VerifyMoveDataInSync(0x2b4c18,0x2b4c90);
 if(m.word(0x3121ec)){for(unsigned i=0;i<14;++i)snap.MoveData(a+0x28b4+4*i,4);snap.MoveData(a+0x28f0,4);}
 snap.VerifyMoveDataInSync(0x2b4c18,0x2b4ca8);
 snap.MoveData(a+0x28f4,1,4,0);snap.MoveData(a+0x28f8,1,4,0);snap.MoveData(a,4,4,1);snap.MoveData(a+0x28fc,4,4,1);snap.MoveData(a+0x2900,4,4,1);
 snap.EndSection();
}
void SnapshotCops(::port::driving::Memory& m,std::uint32_t snapshot){if(const auto manager=m.word(0x328c1c))tCopManager(m,manager).DoSnapshot(snapshot);}
}

namespace hp2::debug {
// Debug0021EC80, E-0099-W107. Exact signed inverse multiply/shift for stride28.
void tCopManager::DoPointerSnapshot(std::uint32_t snapshot,::port::race::SnapshotSpeedTrapPointer pointer){auto& m=memory_;::port::driving::Memory::Temporary local(m,4);const auto trap=m.word(pointer.address);const auto product=(trap-address_-0x2044u)*0xb6db6db7u;m.word(local.address(),trap?std::uint32_t(static_cast<std::int32_t>(product)>>2):0xffffffffu);ReplaySnapshot(m,snapshot).MoveData(local.address(),1,4,1);if(m.byte(snapshot+0x3d)){const auto number=m.signed_word(local.address());m.word(pointer.address,number<0?0:address_+0x2044+28u*std::uint32_t(number));}}
// Debug0021EE40, E-0099-W107. Signed byte ID indexes actual borrowed table.
void tCopManager::DoPointerSnapshot(std::uint32_t snapshot,::port::race::SnapshotPursuitPointer pointer){auto& m=memory_;::port::driving::Memory::Temporary local(m,1);m.byte(local.address(),255);if(!m.byte(snapshot+0x3d))if(const auto pursuit=m.word(pointer.address))m.byte(local.address(),m.byte(pursuit+0x5c8));ReplaySnapshot(m,snapshot).MoveData(local.address(),1);if(m.byte(snapshot+0x3d)){const auto id=m.signed_byte(local.address());m.word(pointer.address,id<0?0:m.word(address_+0x34+4*std::uint32_t(id)));}}
}

#include "game_core/port/race/car_construction.hpp"
namespace hp2::debug {
// Debug00221B28, E-0099-W91; source counters update only after actual ctor completes.
void tCopManager::CreateCop(std::uint32_t driver,bool player,const ::port::race::CarConstructionServices& construction,::port::race::PhysicsConstructionServices& physics){auto& m=memory_;const auto index=m.word(address_+4);if(index>=7)hp2::not_implemented("0x00221B28","original seven-element Cop table index exceeded");if(player){m.word(address_+0xc,index);const auto world=m.word(0x3272f0);if(!world)hp2::not_implemented("0x00221B28","player cop requires actual World");const auto cop=m.allocate(0x34);tPlayerCop(m,cop).Sub_00227BD0(m.word(world+0x2c),int(index));m.word(address_+0x10+4*index,cop);const auto mask=m.word(0x311f84)==0?1u:2u;for(int i=0;i<m.signed_word(address_+0x2040);++i){const auto trap=address_+0x2048+0x1c*std::uint32_t(i);if(m.word(trap+0x14)&mask)hp2::not_implemented("0x00132F48","actual player-cop speedtrap ResetToPosition vector/physics pipeline is required");}}else{const auto cop=bOMalloc(m,m.word(0x328c08));tAICop(m,cop).Sub_002281A8(driver,int(m.word(address_+4)),construction,physics);m.word(address_+0x10+4*index,cop);m.word(address_+4,m.word(address_+4)+1);}m.word(address_+8,m.word(address_+8)+1);}
// Debug00221EE0, E-0099-W91. Original source takes dimensions from first actual cop.
void tCopManager::CalculateCopCarDimensions(){auto& m=memory_;const auto type=m.word(m.word(m.word(address_+0x10)+4)+0xc);const float scale=m.scalar(0x325fd4);m.scalar(address_+0x2908,m.scalar(type+0x1e4)*scale);m.scalar(address_+0x290c,m.scalar(type+0x1e8)*scale);}
// Debug002225B0, E-0099-W91; signed nMaxCops and actual manager owner.
void AddCopCarToRace(::port::driving::Memory& m,std::uint32_t driver,bool player,const ::port::race::CarConstructionServices& construction,::port::race::PhysicsConstructionServices& physics){if(m.signed_word(0x3121d4)<=0)return;const auto manager=m.word(0x328c1c);if(!manager)hp2::not_implemented("0x002225B0","original CopManager owner absent");tCopManager object(m,manager);object.CreateCop(driver,player,construction,physics);object.CalculateCopCarDimensions();}
}

#include "game_core/indep/src/pursuit.hpp"
namespace hp2::debug {
// Debug0021ED68,E-0099-W119. Actual0x640-byte/6-slot Pursuit pool; insert tail.
std::uint32_t tCopManager::CreatePursuit(std::int8_t id,std::uint32_t car){auto& m=memory_;if(id<0||id>=6)hp2::not_implemented("0x0021ED68","original registered PursuitTable ID outside six reviewed slots");const auto pursuit=bOMalloc(m,m.word(0x328c04));tPursuit(m,pursuit).Sub_00223850(id,car);const auto head=address_+0x2c,previous=m.word(head+4);m.word(previous,pursuit);m.word(address_+0x34+4*static_cast<unsigned>(id),pursuit);m.word(head+4,pursuit);m.word(pursuit+4,previous);m.word(pursuit,head);return pursuit;}
// Debug0021EDE8,E-0099-W119. Exact unlink/table clear then signed virtual dtor3.
void tCopManager::DeletePursuit(std::uint32_t pursuit){if(!pursuit)return;auto& m=memory_;const auto id=m.signed_byte(pursuit+0x5c8);if(id<0||id>=6)hp2::not_implemented("0x0021EDE8","original negative/dummy or out-of-table pursuit deletion domain is unresolved");const auto next=m.word(pursuit),previous=m.word(pursuit+4);m.word(previous,next);const auto table=m.word(pursuit+0x634);m.word(next+4,previous);m.word(address_+0x34+4*static_cast<unsigned>(id),0);if(m.word(table+0xc)!=0x223a18)hp2::not_implemented("0x0021EDE8","original pursuit destructor virtual target is not reconstructed");tPursuit(m,pursuit+static_cast<std::int32_t>(m.signed_half(table+8))).Sub_00223A18(3);}
// Debug0021FE88,E-0099-W119. Original four-entry deque push-tail/no-full branch.
void tCopManager::ReleaseCopSpeaker(std::uint32_t speaker){auto& m=memory_;const auto q=address_+0x2844;if(!m.word(q+0x18)){const auto tail=m.word(q+0x14);if(tail>=4)hp2::not_implemented("0x0021FE88","source CopSpeakers tail outside four entries");m.word(q+4*tail,speaker);const auto next=tail+1==4?0:tail+1;m.word(q+0x14,next);m.word(q+0x18,next==m.word(q+0x10));}}
// Debug0021EBE0,E-0099-W119. Original inverse-stride multiply wraps32bits.
void tCopManager::DoPointerSnapshot(std::uint32_t snapshot,::port::race::SnapshotRoadBlockPointer pointer){auto& m=memory_;::port::driving::Memory::Temporary number(m,4);const auto block=m.word(pointer.address);const auto product=(block-address_-0x140u)*0xeeeeeeefu;m.word(number.address(),block?static_cast<std::uint32_t>(static_cast<std::int32_t>(product)>>3):0xffffffffu);ReplaySnapshot(m,snapshot).MoveData(number.address(),1,4,1);if(m.byte(snapshot+0x3d)){const auto id=m.signed_word(number.address());if(id>=64)hp2::not_implemented("0x0021EBE0","original roadblock ID exceeds inline64-record array");m.word(pointer.address,id<0?0:address_+0x140+0x78*static_cast<unsigned>(id));}}
// Debug0021EED0,E-0099-W119. Signed cop number byte, fullword local storage.
void tCopManager::DoPointerSnapshot(std::uint32_t snapshot,::port::race::SnapshotCopPointer pointer){auto& m=memory_;::port::driving::Memory::Temporary number(m,4);const auto cop=m.word(pointer.address);m.word(number.address(),cop?m.word(cop+8):0xffffffffu);ReplaySnapshot(m,snapshot).MoveData(number.address(),1,4,1);if(m.byte(snapshot+0x3d)){const auto id=m.signed_word(number.address());if(id>=7)hp2::not_implemented("0x0021EED0","original cop pointer ID exceeds seven-entry table");m.word(pointer.address,id<0?0:m.word(address_+0x10+4*static_cast<unsigned>(id)));}}
}

#include "game_core/indep/inc/RealDriver.hpp"
#include "game_core/indep/inc/CarController.hpp"
#include "game_core/port/race/car_reset_services.hpp"
namespace hp2::debug {
// Debug00222DA8; E-0110-F02. Source signed car index and ticket byte.
int tCopManager::GetNumTickets(std::uint32_t car)const{return memory_.signed_byte(address_+0x60+0x1cu*std::uint32_t(memory_.signed_half(car+8)));}
// Debug002200F0,E-0099-W121. Actual copmgr.cpp owner; ordinary source timers,
// complete nonarrest release. Reached arrest/BeACop children retain their calls.
void tCopManager::EndPullover(int index,const ::port::race::CarResetServices* reset){
 auto& m=memory_;const auto a=address_;if(index<0||index>=8)hp2::not_implemented("0x002200F0","original pullover index outside eight timer/entry records");
 const auto world=m.word(0x3272f0);if(!world)hp2::not_implemented("0x002200F0","actual World owner required before pullover CarTable lookup");const auto car=m.word(world+0x2c+4*static_cast<unsigned>(index));if(!car)hp2::not_implemented("0x002200F0","actual CarTable owner required by pullover release");const auto entry=a+0x58+0x1c*static_cast<unsigned>(index);
 m.word(entry+0xc,0);m.scalar(a+0x2874+4*static_cast<unsigned>(index),8);const float spawn=GetBaseSpeedTrapSpawnTime();const auto ordinary=[](float value){return std::isfinite(value)&&(value==0||std::isnormal(value));};if(!ordinary(spawn))hp2::not_implemented("0x00220160","original pullover spawn timer MUL.S input outside finite normal/zero domain");const float timer=spawn*.75f;if(!ordinary(timer)||(spawn!=0&&timer==0))hp2::not_implemented("0x00220160","original pullover spawn timer MUL.S result outside supported ordinary domain");m.scalar(a+0x2894+4*static_cast<unsigned>(index),timer);hp2::recovered::Car vehicle(m,car);vehicle.FixBlownTires();if(m.word(car+0x20)==1)hp2::recovered::RealDriver(m,m.word(car+0x2c)).ForceEBrakeStop(0);
 const int cop_index=m.signed_word(entry);if(cop_index!=-1){if(cop_index<0||cop_index>=7)hp2::not_implemented("0x002201B0","original pullover cop index outside seven manager entries");const auto cop=m.word(a+0x10+4*static_cast<unsigned>(cop_index));if(!cop)hp2::not_implemented("0x002201B0","actual pullover cop owner absent");const auto table=m.word(cop+0x1c),self=cop+static_cast<std::int32_t>(m.signed_half(table+0x10));bool controlled;if(m.word(table+0x14)==0x22b380)controlled=tAICop(m,self).IsAIControlled();else if(m.word(table+0x14)==0x22b3b8)controlled=tPlayerCop(m,self).IsAIControlled();else hp2::not_implemented("0x002201B0","original pullover IsAIControlled target is not reconstructed");if(controlled)tAICop(m,cop).DoneWritingTicket();}
 const auto info=m.word(car+0x14);if(!info)hp2::not_implemented("0x002200F0","actual DriverInfo owner required by pullover release");
 if(m.word(entry+0x14)){m.word(a+0x2900,m.word(a+0x2900)+1);if(m.signed_word(info+8)==2){const auto controller=m.word(car+0x28);m.word(controller+0x210,6);m.scalar(controller+0x224,-1);}if(m.word(0x3121ec))hp2::not_implemented("0x002200F0","source BeACop arrest quota/result chain required");if(!reset||!reset->routes)hp2::not_implemented("0x00149028","source arrested car finish requires actual route owner");Race(m,m.word(0x327258)).FinishCar(::port::race::FinishCarPointer{car},-1,false,2,*reset->routes);m.word(car+0x86c,0);m.word(car+0x4ac,0);return;}
 if(m.signed_word(info+8)==2){const auto controller=m.word(car+0x28),table=m.word(controller+8);m.word(controller+0x210,5);const auto self=controller+static_cast<std::int32_t>(m.signed_half(table+0x68));switch(m.word(table+0x6c)){case 0x191458:AICarController(m,self).ReleaseBoost();break;case 0x1bd278:CarController(m,self).ReleaseBoost();break;default:hp2::not_implemented("0x0022031C","original pullover ReleaseBoost virtual target is not reconstructed");}}
 else if(m.word(car+0x18)){NearestRoutePointInfo target;target.Route=vehicle.GetRoute();target.Point=vehicle.GetRoutePoint();GetNextRoutePoint(m,target);if(!reset)hp2::not_implemented("0x00132FB0","actual route/physics owners and independent reset W operands required by player pullover");vehicle.ResetToRoutePoint(target.Route,target.Point,m.scalar(0x326fc8),true,1,*reset);}
 if(m.word(0x3121ec)){const int player_cop=m.signed_word(a+0xc);if(player_cop<0||player_cop>=7)hp2::not_implemented("0x00220384","original BeACop pullover requires actual player cop index");const auto cop=m.word(a+0x10+4*static_cast<unsigned>(player_cop)),copcar=m.word(cop+4);if(m.word(entry+0x18))hp2::not_implemented("0x00223788","actual Player.StopBustedDisplay child required by BeACop pullover");if(m.signed_half(copcar+8)!=index){m.word(copcar+0x834,0);m.word(copcar+0x808,0);if(m.word(0x3121f0))m.scalar(a+0x28f0,static_cast<float>(m.signed_word(0x311f9c))*m.scalar(0x325fc4));if(m.word(car+0x18))hp2::not_implemented("0x00222FD8","actual ResetAllDamage for chased/player-cop Cars required");}}
}
}


#include "game_core/port/race/damage_resources.hpp"
namespace hp2::debug {
// Debug00222FD8,E-0110-P01; original copmgr.cpp. Repair the real damage
// owner, traction override and indexed cop damage; preserve adjacent tire bits.
void tCopManager::ResetAllDamage(std::uint32_t car){auto& m=memory_;hp2::recovered::Car(m,car).FixBlownTires();const int index=m.signed_half(car+8);if(index<0||index>=8)hp2::not_implemented("0x00222FD8","Original CarIndex exceeds eight damage entries");const auto damage=m.word(car+0x844);m.scalar(car+0x45c,1);m.word(address_+0x28b4+4*std::uint32_t(index),0);if(damage)CarDamage(m,damage).SetDamageToZero();if(m.word(car+0x20)==1)hp2::recovered::RealDriver(m,m.word(car+0x2c)).ForceEBrakeStop(0);}
// Debug00223280,E-0110-P01; original copmgr.cpp. Both source bool storage
// slots are read as fullwords and the original scan includes all eight rows.
bool tCopManager::ShouldDisplayBustedText()const{for(unsigned i=0;i<8;++i)if(memory_.word(address_+0x64+28*i)&&memory_.word(address_+0x70+28*i))return true;return false;}
}

namespace hp2::debug {
// Debug0021ED28/0021FD60/0021FDC8,E-0100-PA01. Source table/deque queries.
std::int8_t tCopManager::GetNextPursuitID(){for(int i=0;i<6;++i)if(!memory_.word(address_+0x34+4*unsigned(i)))return std::int8_t(i);return -1;}
std::uint32_t tCopManager::GetCurrentCopSpeaker(){const auto q=address_+0x2844,head=memory_.word(q+0x10);if(!memory_.word(q+0x18)&&head==memory_.word(q+0x14))return 0;if(head>=4)hp2::not_implemented("0x0021FD60","source CopSpeakers head outside four-entry deque");return memory_.word(q+4*head);}
void tCopManager::NextCopSpeaker(){const auto q=address_+0x2844;if(memory_.word(q+0x18)||memory_.word(q+0x10)!=memory_.word(q+0x14))ReleaseCopSpeaker(ReserveCopSpeaker());}
// Debug0021FC60,E-0100-PA01. Optional speech emission does not remove the
// original supervisor probability, replay eligibility or seven-second timer.
void tCopManager::PossibleCallSupervisor(std::uint32_t pursuit,bool disable_speech){auto& m=memory_;if(::port::race::cop_frame_replay_mode(m,"0x0021FC60"))return;if(std::fabs(m.scalar(address_+0x286c)+50.f)<=m.scalar(0x325fc0)&&m.word(0x3121dc)&&m.word(m.word(pursuit+0x10)+0x18)&&hp2::recovered::bRandom(m,2)==0){(void)tPursuit(m,pursuit).GetCopSpeaker();if(!disable_speech)hp2::not_implemented("0x00206370","original raiseSuperRequest speech event submission required");::port::diagnostics::warn("0x00206370","supervisor request speech emission disabled by explicit PC presentation policy","retaining original replay/roadblock eligibility, shared RNG draw and seven-second supervisor timer");m.scalar(address_+0x286c,7.f);}}
// Debug0021F8F0,E-0100-PA01. Preserve original admission limits, list scan,
// source creation/speech/NIS order, array append, and pointer-link publication.
bool tCopManager::RequestPursuit(std::uint32_t cop,std::uint32_t target,::port::race::PhysicsConstructionServices* physics){
 auto& m=memory_;const auto racing=m.word(target+0x1c);if(!racing)hp2::not_implemented("0x0021F8F0","original pursuit target lacks RacingCar owner");if(m.signed_half(racing+0x12))return false;
 int maximum_cops=0,maximum_roadblock=0;bool allowed=false;std::uint32_t pursuit=0;
 if(m.word(0x3121ec)){const int player_index=m.signed_word(address_+0xc);std::uint32_t player_cop=0;if(player_index!=-1){if(player_index<0||player_index>=7)hp2::not_implemented("0x0021F8F0","source player cop table index outside seven entries");player_cop=m.word(address_+0x10+4*unsigned(player_index));}if(cop==player_cop){maximum_cops=1;maximum_roadblock=2;allowed=true;}}
 else{int npc_count=0;const auto head=address_+0x2c;for(auto current=m.word(head);current!=head;current=m.word(current)){const auto chased=m.word(current+0x10);if(!m.word(chased+0x18))++npc_count;if(chased==target)pursuit=current;}if(pursuit&&m.word(pursuit+0x5cc)==5)return false;
  const bool human=m.word(target+0x18)!=0;if(m.signed_word(0x311fa8)>=2){maximum_cops=1;maximum_roadblock=human?1:0;allowed=pursuit?m.signed_word(pursuit+0x5c4)<1:human||npc_count==0;}
  else if(!human){maximum_cops=1;allowed=pursuit?m.signed_word(pursuit+0x5c4)<1:(m.word(0x2e78e0)?npc_count==0:npc_count<2);}
  else{const auto world=m.word(0x3272f0);if(!world)hp2::not_implemented("0x0021F8F0","actual World CopCarPos required by player pursuit limits");maximum_cops=m.signed_word(world+0x24)>2?1:2;maximum_roadblock=2;const int count=pursuit?m.signed_word(pursuit+0x5c4):0;allowed=!pursuit||count==0||(count<maximum_cops&&m.word(pursuit+0x5ec));}
 }
 if(!allowed)return false;const bool disable_speech=physics&&physics->disable_pursuit_speech_emission;
 if(pursuit)tPursuit(m,pursuit).JoinPursuitSpeech(disable_speech);
 else{pursuit=CreatePursuit(GetNextPursuitID(),target);tPursuit(m,pursuit).StartPursuitSpeech(disable_speech);const auto subject=m.word(0x3121ec)?m.word(cop+4):m.word(pursuit+0x10);if(const auto player=m.word(subject+0x18))Player(m,player).LoadNextNis(m.word(pursuit+0x10),m.word(cop+4),GetNumTickets(m.word(pursuit+0x10)),physics&&physics->disable_pursuit_nis_asset_load);}
 const int count=m.signed_word(pursuit+0x5c4);if(count<0||count>=2)hp2::not_implemented("0x0021FB44","source pursuit cop append exceeds original two-pointer array");m.word(pursuit+8,std::uint32_t(maximum_cops));m.word(pursuit+0xc,std::uint32_t(maximum_roadblock));m.word(pursuit+0x1c+4*unsigned(count),cop);m.word(pursuit+0x5c4,std::uint32_t(count+1));tCop(m,cop).SetPursuit(pursuit);return true;
}
}

// Debug002205A0, E-0100-PU02. Actual fleet/state/NIS and XY distance gate.
namespace hp2::debug {
bool tCopManager::IsCopNearby(std::uint32_t car,float distance){auto& m=memory_;if(m.word(0x3121ec))hp2::not_implemented("0x002205A0","actual BeACop player-cop range predicate required");for(int i=0;i<m.signed_word(address_+8);++i){if(i>=7)hp2::not_implemented("0x002205A0","original cop fleet exceeds seven owned entries");const auto cop=m.word(address_+0x10+4*i);const int state=m.signed_word(cop+0x10);if(state!=2&&state!=3&&state!=4&&state!=5&&state!=6&&state!=8)continue;const auto other=m.word(cop+4),player=m.word(other+0x18);if(player&&Player(m,player).IsNisPlaying())continue;const float x=m.scalar(other+0x60)-m.scalar(car+0x60),y=m.scalar(other+0x64)-m.scalar(car+0x64);if(std::sqrt(x*x+y*y)<=distance)return true;}return false;}
}

// Debug00222F40,E-0100-PU03. Scan actual fleet in source order, no synthetic
// age, speed or collision-attribution filter.
namespace hp2::debug {
bool tCopManager::DidRecentlyCollideWithCop(std::uint32_t car){
 auto& m=memory_;
 for(int i=0;i<m.signed_word(address_+8);++i){
  if(i>=7)hp2::not_implemented("0x00222F40","original cop fleet exceeds seven owned entries");
  if(DidTheseCarsCollide(m,car,m.word(m.word(address_+0x10+4*i)+4)))return true;
 }
 return false;
}
}

#include "game_core/port/race/cop_capture.inc"
#include "game_core/port/race/cop_escalation.inc"
