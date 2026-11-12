// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/src/trackgraph.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/inc/bSlotPool.hpp"
#include <stdexcept>
#include <utility>

namespace hp2::recovered {
// ELF 0x0024E4D0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0024e4d0.md.
void tTrackGraph::GetVertexNumber() {
    hp2::not_implemented("0x0024E4D0", "tTrackGraph::GetVertexNumber (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace port::track {
// Host bounds around the recovered memory access, not original failure semantics.
static std::uint32_t graph_signpost(const RouteData& data,const port::driving::Memory& m,std::uint32_t point){
    const auto index=m.byte(point+0x2c);
    if(index>=data.signpost_count || !data.signposts)throw std::out_of_range("Original graph signpost index invalid");
    return data.signposts+8*index;
}
}
namespace hp2::debug {
using port::driving::Memory;
// Debug00248150/00248168; E-0099-W07. Capacity guards are port-only.
std::uint32_t tTrackGraph::GetVertex(int i)const{
    if(i<0||i>=100)throw std::out_of_range("Original graph vertex capacity exceeded");
    return address_+0xc+std::uint32_t(i)*0x28;
}
std::uint32_t tTrackGraph::GetEdge(int i)const{
    if(i<0||i>=400)throw std::out_of_range("Original graph edge capacity exceeded");
    return address_+0xfac+std::uint32_t(i)*0x2c;
}
// Debug002478E0; E-0099-W07. Real TrackPathCoordinate supplies distance.
void tTrackGraph::AddVertex(std::uint32_t route,std::uint32_t point,const port::track::RouteData& data){
    auto& m=memory_;const auto index=m.signed_word(address_);const auto v=GetVertex(index);
    m.word(address_,std::uint32_t(index+1));m.word(v+0x18,0xffffffffu);m.word(v+0x1c,0xffffffffu);
    m.word(v+8,port::track::graph_signpost(data,m,point));
    const int number=m.signed_half(route+0xa);
    m.word(v+0xc,std::uint32_t(number));
    const auto offset=std::int64_t(point)-route-0x428;
    if(offset<0 || offset%0x70)throw std::out_of_range("Original graph point is not a route table entry");
    const int n=int(offset/0x70);m.word(v+0x10,n);
    m.scalar(v+0x14,port::track::route_point_distance(data,m,number,n));
}
// Debug00247990; E-0099-W07. Match own endpoint OR signpost endpoint in order.
int tTrackGraph::GetVertexNumber(int route,int point)const{
    const auto& m=memory_;
    for(int i=0;i<m.signed_word(address_);++i){const auto v=GetVertex(i);
        if(m.signed_word(v+0xc)==route && m.signed_word(v+0x10)==point)return i;
        const auto sign=m.word(v+8);if(m.byte(sign)==route && m.half(sign+2)==point)return i;
    }
    return -1;
}
// Debug00247A20; E-0099-W07. Append slots BEFORE detecting the destination,
// so the ending route point contributes its roadblock/speedtrap records as well.
int tTrackGraph::AddEdge(int vertex,bool branch,bool cloaked,const port::track::RouteData& data){
    auto& m=memory_;const int index=m.signed_word(address_+4);const auto edge=GetEdge(index),v=GetVertex(vertex);
    m.word(address_+4,index+1);m.word(edge+4,0xffffffffu);m.word(edge+0x18,cloaked);m.word(edge,vertex);m.word(edge+0x14,cloaked);
    const auto sign=m.word(v+8);const int route=branch?m.byte(sign):m.signed_word(v+0xc);
    const int first=branch?m.half(sign+2):m.signed_word(v+0x10);
    m.word(edge+8,route);m.word(edge+0xc,first);
    if(route<0 || route>=100 || !data.routes[route])throw std::out_of_range("Original graph edge route missing: vertex="+std::to_string(vertex)+" route="+std::to_string(route)+" first="+std::to_string(first)+" own_route="+std::to_string(m.signed_word(v+0xc))+" sign_bits="+std::to_string(m.word(sign)));
    const auto route_address=data.routes[route];const int count=m.signed_word(route_address+0x10);
    if(first<0)throw std::out_of_range("Original graph edge first point invalid");
    const auto append=[&](std::uint32_t head,std::uint32_t pool,std::uint32_t value){
        const auto node=bOMalloc(m,m.word(pool));if(!node)hp2::not_implemented("0x00247A20","original graph slot allocation failed");
        const auto tail=m.word(head+4);m.word(tail,node);m.word(head+4,node);m.word(node+8,value);m.word(node+4,tail);m.word(node,head);
    };
    for(int p=first;p<count;++p){const auto point=route_address+0x428+std::uint32_t(p)*0x70;const auto flags=m.word(point+0x2c);
        if((flags&0x3f00)!=0x3f00)append(edge+0x1c,0x328ef0,(flags>>8)&0x3f);
        if((flags&0xfc00000)!=0xfc00000)append(edge+0x24,0x328ef4,(flags>>22)&0x3f);
        if(p!=first && m.byte(point+0x2c)!=0xff){const int to=GetVertexNumber(route,p);
            if(to!=-1){m.word(edge+0x10,p);m.word(edge+4,to);return index;}}
    }
    // The original leaves nLastRoutePoint untouched if no endpoint was found.
    return index;
}
// Debug00247218; E-0099-W07. Original order,100 vertices/400 edge capacity,
//49 racing-route scan, branch rules, incoming overwrite and backward ordering.
void tTrackGraph::Sub_00247218(const port::track::RouteData& data){
    auto& m=memory_;m.word(address_+0x546c,0x2b6e58);
    for(int i=0;i<400;++i){const auto e=GetEdge(i);for(auto h:{e+0x1c,e+0x24}){m.word(h,h);m.word(h+4,h);}}
    m.word(address_,0);m.word(address_+4,0);m.word(address_+8,0);
    m.word(0x328ef0,bNewSlotPool(m,0xc,0x80,0x2b6e38,0x10));
    m.word(0x328ef4,bNewSlotPool(m,0xc,0x80,0x2b6e48,0x10));
    for(int r=0;r<49;++r){const auto route=data.routes[r];if(!route)continue;
        for(int n=0;n<m.signed_word(route+0x10);++n){const auto p=route+0x428+std::uint32_t(n)*0x70;
            // Listing00247348 is LBU/XORI FF; the decompiler's field != -1
            // refers to the 8-bit SignNumber, not the packed 32-bit word.
            if(m.byte(p+0x2c)==0xff)continue;
            const auto bits=m.word(port::track::graph_signpost(data,m,p));
            if(!(bits&0x800) || (bits&0x700)==0x300){if(bits&0x4000)m.word(address_+8,1);AddVertex(route,p,data);}
        }
    }
    for(int i=0;i<m.signed_word(address_);++i){const auto v=GetVertex(i);const auto bits=m.word(m.word(v+8));const auto kind=bits&0x700;
        if(kind!=0x300 || (bits&0x800)){
            if(kind==0x200 || kind==0x300)m.word(v+0x18,AddEdge(i,true,false,data));
            else{m.word(v+0x18,AddEdge(i,false,false,data));m.word(v+0x1c,AddEdge(i,true,(bits&0x4000)!=0,data));}
        }
    }
    for(int i=0;i<m.signed_word(address_);++i){const auto v=GetVertex(i);m.word(v+0x20,0xffffffffu);m.word(v+0x24,0xffffffffu);}
    for(int i=0;i<m.signed_word(address_+4);++i){const auto v=GetVertex(m.signed_word(GetEdge(i)+4));m.word(v+0x20+(m.signed_word(v+0x20)!=-1?4:0),i);}
    for(int i=0;i<m.signed_word(address_);++i){const auto v=GetVertex(i);const auto back=m.signed_word(v+0x24),forward=m.signed_word(v+0x18);
        if(back!=-1 && forward!=-1 && m.word(GetEdge(back)+0x14)==0 && m.word(GetEdge(back)+8)==m.word(GetEdge(forward)+8)){
            const auto old=m.word(v+0x20);m.word(v+0x20,back);m.word(v+0x24,old);
        }
    }
}
// Debug00247878; E-0099-W07. Shared Signpost mutation is part of uncloaking.
void tTrackGraph::UncloakAllEdges(){
    auto& m=memory_;for(int i=0;i<m.signed_word(address_+4);++i){const auto e=GetEdge(i);
        if(m.word(e+0x14)){m.word(e+0x14,0);const auto sign=m.word(GetVertex(m.signed_word(e))+8);m.word(sign,m.word(sign)&0xffffbfffu);}}
    m.word(address_+8,0);
}
// Debug00247C30 /00248148; E-0099-W07.
std::uint32_t tTrackGraph::FindEdgeByRouteAndPoint(int route,int point)const{
    const auto& m=memory_;for(int i=0;i<m.signed_word(address_+4);++i){const auto e=GetEdge(i);
        if(!m.word(e+0x14) && m.signed_word(e+8)==route && m.signed_word(e+0xc)<=point && point<=m.signed_word(e+0x10))return e;}
    return 0;
}
bool tTrackGraph::HasCloakedEdges()const{return (memory_.word(address_+8)&0xff)!=0;}
// Debug00247608; E-0099-W07. Lists free through their original pools before
// pool teardown. Empty list/node destructors have no remaining store, confirmed
// by00248180/00248188/00248290/00248308 and bNode00115280.
void tTrackGraph::Sub_00247608(int in_chrg){
    auto& m=memory_;m.word(address_+0x546c,0x2b6e58);
    for(int i=0;i<m.signed_word(address_+4);++i){const auto e=GetEdge(i);
        for(int k=0;k<2;++k){const auto head=e+(k?0x24:0x1c),pool=m.word(k?0x328ef4:0x328ef0);
            while(m.word(head)!=head){const auto node=m.word(head),next=m.word(node),previous=m.word(node+4);
                m.word(previous,next);m.word(next+4,previous);SlotPool(m,pool).Free(node);}
        }
    }
    bDeleteSlotPool(m,m.word(0x328ef0));m.word(0x328ef0,0);bDeleteSlotPool(m,m.word(0x328ef4));m.word(0x328ef4,0);
    if(in_chrg&1)m.release(address_);
}
// Debug002480D0 /00248100; E-0099-W07.
void InitTrackGraph(Memory& m,const port::track::RouteData& data){
    if(!data.initialized)throw std::logic_error("Original route postload must precede graph initialization");
    const auto graph=m.allocate(0x5470);tTrackGraph(m,graph).Sub_00247218(data);m.word(0x328ef8,graph);
}
void CleanupTrackGraph(Memory& m){if(const auto graph=m.word(0x328ef8))tTrackGraph(m,graph).Sub_00247608(3);m.word(0x328ef8,0);}
}

#include "game_core/indep/inc/Replay.hpp"
namespace hp2::debug {
// Debug002477D8, E-0099-W76. Original bool carrier is4bytes with1serialized byte.
void tTrackGraph::DoSnapshot(std::uint32_t snapshot){auto& m=memory_;ReplaySnapshot s(m,snapshot);s.MoveData(address_+8,1,4,0);for(int i=0;i<m.signed_word(address_+4);++i){const auto edge=GetEdge(i);if(m.word(edge+0x18))s.MoveData(edge+0x14,1,4,0);}}
}

#include "game_core/indep/inc/Car.hpp"
#include "game_core/indep/inc/CopChase.hpp"
#include "game_core/indep/src/copmgr.hpp"
namespace hp2::debug {
// Debug00247F00,E-0110-F07. Original authored edge lists; first match, at most five edges.
std::uint32_t tTrackGraph::FindSpeedTrap(std::uint32_t car,float minimum,float maximum,bool forward,const ::port::race::AiDriveTargetServices& ai)const{
 auto& m=memory_;hp2::recovered::Car vehicle(m,car);const auto route=vehicle.GetRoute(ai),point=vehicle.GetRoutePoint(ai);if(!route||!point)return 0;
 const auto difference=static_cast<std::int32_t>(point-route-0x428u);const auto index=static_cast<std::int32_t>(static_cast<std::uint32_t>(difference)*0xb6db6db7u)>>4;
 auto edge=FindEdgeByRouteAndPoint(m.signed_half(route+0xa),index);if(!edge)return 0;const float position=m.scalar(car+0x7f4);
 for(int visited=0;visited<5;++visited){const auto head=edge+0x24;for(auto node=m.word(head);node!=head;node=m.word(node)){const auto manager=m.word(0x328c1c);if(!manager)hp2::not_implemented("0x00221420","actual CopManager required by authored graph trap index");const auto trap=tCopManager(m,manager).GetSpeedTrapFromLoadedDataIndex(m.signed_word(node+8));if(!trap)continue;const float distance=forward?GetForwardTrackDistance(m,m.scalar(trap+0x14),position):GetForwardTrackDistance(m,position,m.scalar(trap+0x14));if(distance<maximum&&minimum<=distance)return trap;}
 if(visited==4)break;const auto vertex=GetVertex(m.signed_word(edge+(forward?4:0)));const int next=m.signed_word(vertex+(forward?0x18:0x20));if(next==-1)break;edge=GetEdge(next);}
 return 0;
}
}
