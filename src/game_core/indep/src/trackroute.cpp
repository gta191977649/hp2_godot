// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\trackroute.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): TrackRoute::FindClosestPoint() [0x002B7328]
#include "game_core/indep/inc/TrackRoute.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/port/track/route_data.hpp"
#include "game_core/indep/src/trackpath.hpp"
#include "game_core/port/assets/elab_chunks.hpp"
#include <cmath>
#include <cstring>
#include <functional>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <limits>
#include "game_core/port/driving/projections.hpp"

namespace hp2::recovered {
// ELF 0x001C1910; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c1910.md.
void TrackRoute::GetRelativePoint() {
    hp2::not_implemented("0x001C1910", "TrackRoute::GetRelativePoint (signature and full body unresolved)");
}

// ELF 0x001C1D30; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c1d30.md.
void TrackRoute::FindClosestPointAhead() {
    hp2::not_implemented("0x001C1D30", "TrackRoute::FindClosestPointAhead (signature and full body unresolved)");
}

// ELF 0x001C2780; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c2780.md.
void TrackRoute::DistanceIsWithinRoute() {
    hp2::not_implemented("0x001C2780", "TrackRoute::DistanceIsWithinRoute (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace port::track {
namespace {
std::uint32_t route_word(const std::vector<std::uint8_t>& bytes,std::size_t at){
    if(at>bytes.size()||bytes.size()-at<4)throw std::runtime_error("Truncated original route data");
    return bytes[at]|std::uint32_t(bytes[at+1])<<8|std::uint32_t(bytes[at+2])<<16|std::uint32_t(bytes[at+3])<<24;
}
int route_half(const std::vector<std::uint8_t>& bytes,std::size_t at){
    if(at>bytes.size()||bytes.size()-at<2)throw std::runtime_error("Truncated original route index");
    return std::int16_t(std::uint16_t(bytes[at])|std::uint16_t(bytes[at+1])<<8);
}
float route_scalar(const std::vector<std::uint8_t>& bytes,std::size_t at){
    const auto bits=route_word(bytes,at);float value;std::memcpy(&value,&bits,4);
    if(!std::isfinite(value))throw std::runtime_error("Nonfinite original route scalar");
    return value;
}
}
// Original byte schemas Debug001BFFC8/001BF9B0/002029C0; E-0099-W08.
// Original list/table writes Debug001BF8A0/001BF950. Host allocation and
// bounded input validation are port-owned; no native pointer overlay is used.
RouteData load_retail_route_data(port::driving::Memory& memory,const std::vector<std::uint8_t>& bytes,bool reverse){
    RouteData data;data.paths.reverse_direction=reverse;const auto chunks=hp2::assets::parse_chunks_top(bytes);
    const auto* path_chunk=hp2::assets::find_first(chunks,0x80034147);
    if(!path_chunk||path_chunk->children.size()<2)throw std::runtime_error("Original track path chunks missing");
    const auto& rows=path_chunk->children[0];const auto& points=path_chunk->children[1];
    if(rows.tag!=0x34148||points.tag!=0x34149||rows.size%40||points.size%20)
        throw std::runtime_error("Original track path schema mismatch");
    for(auto at=points.payload_offset();at<points.payload_offset()+points.size;at+=20){
        PathPoint p;p.position={{route_scalar(bytes,at),route_scalar(bytes,at+4)}};
        p.path=std::int8_t(bytes.at(at+8));for(int n=0;n<3;++n)p.neighbours[n]=route_half(bytes,at+10+2*n);
        data.paths.geometry.points.push_back(p);
    }
    for(auto at=rows.payload_offset();at<rows.payload_offset()+rows.size;at+=40){
        PathRow p;p.begin=route_half(bytes,at+2);p.end=route_half(bytes,at+4);
        const int count=route_half(bytes,at+6);p.spline.looping=bytes.at(at+1)!=0;
        if(p.begin<0||p.end<=p.begin||std::size_t(p.end)>data.paths.geometry.points.size()||count!=p.end-p.begin)
            throw std::runtime_error("Original track path point range invalid");
        for(int i=p.begin;i<p.end;++i)p.spline.points.push_back(data.paths.geometry.points[i].position);
        data.paths.geometry.paths.push_back(std::move(p));RoutePathDistance distance;
        distance.begin=route_scalar(bytes,at+8);distance.end=route_scalar(bytes,at+12);distance.ratio=route_scalar(bytes,at+16);
        data.paths.distances.push_back(std::move(distance));
    }
    for(const auto& p:data.paths.geometry.points){
        if(p.path<0||std::size_t(p.path)>=data.paths.geometry.paths.size())throw std::runtime_error("Original path number invalid");
        for(int n:p.neighbours)if(n>=0&&std::size_t(n)>=data.paths.geometry.points.size())throw std::runtime_error("Original path neighbour invalid");
    }
    hp2::debug::TrackPathManager::Init(data.paths);
    // E-0099-W26. Preserve the exact shipped20-byte point records in shared
    // logical storage. TrackPathCoordinate.pTrackPoint points into this table.
    data.paths.point_table=memory.allocate(static_cast<std::uint32_t>(points.size));
    std::memcpy(memory.view(data.paths.point_table,points.size).data,bytes.data()+points.payload_offset(),points.size);
    // Debug00202780 writes the original manager's TrackLength. Other recovered
    // consumers (including cop spawn timing) read this same shared scalar.
    memory.scalar(0x00313308,data.paths.track_length);
    // Validate all records before publishing the shared manager table/lists.
    std::vector<std::pair<std::size_t,std::size_t>> records;
    const hp2::assets::Chunk* signs=nullptr;
    std::function<void(const std::vector<hp2::assets::Chunk>&)> visit=[&](const auto& list){
        for(const auto& chunk:list){if(chunk.tag==0x34122){if(signs)throw std::runtime_error("Duplicate original signpost chunk");signs=&chunk;}
            if(chunk.tag==0x34121){const auto end=chunk.payload_offset()+chunk.size;
                for(auto at=chunk.payload_offset();at<end;){
                    if(end-at<0x428)throw std::runtime_error("Truncated original route header");
                    const auto count=route_word(bytes,at+0x10);const auto number=route_half(bytes,at+0xa);
                    if(count>1024||number<0||number>=100)throw std::runtime_error("Invalid original route count or number");
                    const auto size=std::size_t(0x428)+std::size_t(count)*0x70;
                    if(size>end-at)throw std::runtime_error("Truncated original route points");
                    for(const auto& r:records)if(route_half(bytes,r.first+0xa)==number)throw std::runtime_error("Duplicate original route number");
                    records.emplace_back(at,size);at+=size;
                }
            }
            visit(chunk.children);
        }
    };visit(chunks);
    if(records.empty()||!signs||signs->size%12)throw std::runtime_error("Original Retail routes/signposts missing or malformed");
    constexpr std::uint32_t manager=0x30bb30;
    memory.view(manager,0x1a0);memory.word(manager,manager);memory.word(manager+4,manager);
    memory.word(manager+8,manager+8);memory.word(manager+12,manager+8);
    for(int i=0;i<100;++i)memory.word(manager+0x10+i*4,0);
    for(const auto& record:records){const auto at=record.first;const auto size=std::uint32_t(record.second);
        const auto address=memory.allocate(size);std::memcpy(memory.view(address,size).data,bytes.data()+at,size);
        const int number=route_half(bytes,at+0xa);data.routes[number]=address;memory.word(manager+0x10+number*4,address);
        const auto type=route_word(bytes,at+0xc);
        if(type==0||type==1){const auto head=manager+(type?8:0);const auto tail=memory.word(head+4);
            memory.word(tail,address);memory.word(head+4,address);memory.word(address,head);memory.word(address+4,tail);}
    }
    // Retail001C2F18 /001C0FF8 /001C1020 /001C1138 /001C14F8 versus
    // Debug001BFFC8 /001BE0C0 /001BE0E8 /001BE1F0 /001BE5A8 prove the shared
    // packed endpoint word and Distance+4, with12-byte versus8-byte stride.
    // The extra Retail word is retained verbatim in port metadata, not guessed.
    data.signpost_count=signs->size/12;
    if(data.signpost_count>255)throw std::runtime_error("Retail signpost count exceeds original8-bit SignNumber domain");
    if(data.signpost_count){data.signposts=memory.allocate(std::uint32_t(data.signpost_count*8));
        data.retail_signposts.resize(data.signpost_count);data.retail_signpost_order.resize(data.signpost_count);
        for(std::size_t i=0;i<data.signpost_count;++i){
            const auto source=bytes.data()+signs->payload_offset()+i*12;
            std::memcpy(data.retail_signposts[i].data(),source,12);
            std::memcpy(memory.view(data.signposts+std::uint32_t(i*8),8).data,source,8);
            data.retail_signpost_order[i]=std::uint32_t(i);
        }
    }
    memory.word(0x328190,data.signposts);memory.word(0x328194,std::uint32_t(data.signpost_count));
    return data;
}
// Debug002478E0's actual002035D8 path projection, E-0099-W08.
float route_point_distance(const RouteData& data,const port::driving::Memory& memory,int route,int point){
    if(route<0||route>=100||!data.routes.at(route))throw std::out_of_range("Original graph route absent");
    const auto address=data.routes.at(route);
    if(point<0||point>=memory.signed_word(address+0x10))throw std::out_of_range("Original graph route point invalid");
    const auto p=address+0x428+std::uint32_t(point)*0x70;
    return hp2::debug::TrackPathCoordinate(data.paths,{{memory.scalar(p),memory.scalar(p+4)}},memory.scalar(0x325da0)).DistanceAroundTrack;
}
}

#include "game_core/indep/src/trackroute.hpp"

namespace hp2::recovered {
// ELF 0x001C28A0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c28a0.md.
void TrackRouteManager::AddRouteToRightList() {
    hp2::not_implemented("0x001C28A0", "TrackRouteManager::AddRouteToRightList (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001C1340; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c1340.md.
void TrackRoutePoint::Sub_001C1340() {
    hp2::not_implemented("0x001C1340", "TrackRoutePoint::Sub_001C1340 (signature and full body unresolved)");
}

// ELF 0x001C1408; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c1408.md.
void TrackRoutePoint::GetPosition() {
    hp2::not_implemented("0x001C1408", "TrackRoutePoint::GetPosition (signature and full body unresolved)");
}

// ELF 0x001C1490; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c1490.md.
void TrackRoutePoint::SetWidth() {
    hp2::not_implemented("0x001C1490", "TrackRoutePoint::SetWidth (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace port::track {
// Port-only validation at logical-pointer boundaries, not original failure rules.
static std::uint32_t checked_route_point(const RouteData& data,const port::driving::Memory& memory,unsigned route,unsigned point){
    if(route>=data.routes.size()||!data.routes[route])throw std::out_of_range("Original signpost target route missing");
    const auto address=data.routes[route];
    if(point>=memory.word(address+0x10))throw std::out_of_range("Original signpost target point missing");
    return address+0x428+point*0x70;
}
static std::uint32_t checked_sign(const RouteData& data,const port::driving::Memory& memory,std::uint32_t point){
    const auto index=memory.byte(point+0x2c);
    if(index>=data.signpost_count||!data.signposts)throw std::out_of_range("Original route point sign index missing");
    return data.signposts+index*8;
}
}
namespace hp2::debug {
// Debug001C0088, E-0099-W120; original trackroute.cpp definition.
bool HasSignposts(::port::driving::Memory& m){return m.word(0x328190)!=0;}
using port::driving::Memory;
// Debug001BE440, E-0099-W17. Lateral contribution is added before along-route
// contribution, preserving original VU grouping rather than a spline shortcut.
std::array<float,2> TrackRoutePoint::GetPosition(float d,float l)const{
    const float dx=memory_.scalar(address_+0xc),dy=memory_.scalar(address_+0x10);
    const float x=dy*l+memory_.scalar(address_),y=(-dx)*l+memory_.scalar(address_+4);
    return {{dx*d+x,dy*d+y}};
}
// Debug001BE750, E-0099-W17. Listing MAX.S chooses the larger of XY distance
// and wrapped track-distance difference minus24; the original is not just XY.
float TrackRoutePoint::GetDistanceAway(const std::array<float,2>& pos,float dat,float length)const{
    const float x=memory_.scalar(address_)-pos[0],y=memory_.scalar(address_+4)-pos[1];
    const float distance=std::sqrt(x*x+y*y);
    const float around=std::abs(TrackPathManager::CompareDistances(length,memory_.scalar(address_+0x28),dat))-24;
    return std::max(distance,around);
}
// Debug001BEBF8, E-0099-W17. Bounds, strict acceptance and distance-derived
// index skipping are original. Host checks reject unsupported malformed ranges.
std::uint32_t TrackRoute::FindClosestPoint(const std::array<float,2>& pos,float maximum,int flags,float dat,float length)const{
    auto& memory=memory_;
    if(!(memory.scalar(address_+0x418)-maximum<=pos[0]&&pos[0]<=memory.scalar(address_+0x420)+maximum&&
         memory.scalar(address_+0x41c)-maximum<=pos[1]&&pos[1]<=memory.scalar(address_+0x424)+maximum))return 0;
    std::uint32_t result=0;const auto count=memory.signed_word(address_+0x10);
    for(int i=0;i<count;){const auto point=address_+0x428+std::uint32_t(i)*0x70;
        const float x=memory.scalar(point),y=memory.scalar(point+4),px=pos[0],py=pos[1];
        const float delta_x=x-px,delta_y=y-py;
        const float distance=(flags&0x20)?TrackRoutePoint(memory,point).GetDistanceAway(pos,dat,length):std::sqrt(delta_x*delta_x+delta_y*delta_y);
        const auto bits=memory.word(point+0x2c);
        if((!(flags&1)||0< -((px-x)*memory.scalar(point+0xc)+(py-y)*memory.scalar(point+0x10)))&&distance<maximum&&
           (!(flags&2)||(bits&0x8000))&&(!(flags&4)||!(bits&0x4000))){maximum=distance;result=point;}
        const int skip=::port::driving::cvt_w_s(distance*memory.scalar(0x00325924))-2;
        const auto next=std::int64_t(i)+(skip>0?skip:0)+1;
        if(next>std::numeric_limits<int>::max())throw std::out_of_range("Original route search skip outside signed range");
        i=int(next);
    }
    return result;
}
// Debug001BE5C0, E-0099-W14. Only core0x30 bytes swap; driving-line data stays
// in its original point slot until ReversePoint performs the original exchanges.
void TrackRoutePoint::SwapPointData(std::uint32_t other){
    std::array<std::uint8_t,0x30> temporary{};
    for(unsigned i=0;i<temporary.size();++i)temporary[i]=memory_.byte(address_+i);
    memory_.copy(address_,other,temporary.size());
    for(unsigned i=0;i<temporary.size();++i)memory_.byte(other+i,temporary[i]);
}
// Debug001BE620 ->001BE4B8, E-0099-W14. Position uses original multiply/add
// grouping, including the zero lateral term;21st driving line remains untouched.
void TrackRoutePoint::ReversePoint(float track_length){
    const auto p=address_;auto& memory=memory_;
    const float length=memory.scalar(p+0x14),dx=memory.scalar(p+0xc),dy=memory.scalar(p+0x10);
    const float normal[3]{dy,-dx,memory.scalar(p+0x20)};
    const float direction[3]{dx,dy,memory.scalar(p+0x24)};
    for(unsigned i=0;i<3;++i){const float lateral=normal[i]*0.0f;const float base=lateral+memory.scalar(p+4*i);
        const float along=direction[i]*length;memory.scalar(p+4*i,along+base);}
    memory.scalar(p+0xc,-dx);memory.scalar(p+0x10,-dy);
    const auto right=memory.word(p+0x1c);memory.word(p+0x1c,memory.word(p+0x18));
    memory.scalar(p+0x24,-memory.scalar(p+0x24));memory.scalar(p+0x20,-memory.scalar(p+0x20));memory.word(p+0x18,right);
    memory.scalar(p+0x28,track_length-memory.scalar(p+0x28));
    for(unsigned i=0;i<10;++i){const auto a=p+0x30+i*2,b=a+20;
        const auto offset=memory.half(a);const auto speed=memory.byte(p+0x5a+i);
        memory.half(a,memory.half(b));memory.byte(p+0x5a+i,memory.byte(p+0x5a+i+10));
        memory.half(b,offset);memory.byte(p+0x5a+i+10,speed);}
}
// Debug001BE738, E-0099-W14. Traffic points reverse distance only.
void TrackRoutePoint::ReverseTrafficRoutePoint(float track_length){memory_.scalar(address_+0x28,track_length-memory_.scalar(address_+0x28));}
// Debug001BF3E8, E-0099-W14. Reverse core point order, then original point
// transforms, then driving-line info halves0..9 and10..19; index20 is unchanged.
void TrackRoute::ReverseRoute(float track_length){
    const auto count=memory_.signed_word(address_+0x10);
    for(int i=0;i<count/2;++i)TrackRoutePoint(memory_,address_+0x428+std::uint32_t(i)*0x70).SwapPointData(address_+0x428+std::uint32_t(count-i-1)*0x70);
    for(int i=0;i<count;++i)TrackRoutePoint(memory_,address_+0x428+std::uint32_t(i)*0x70).ReversePoint(track_length);
    for(unsigned i=0;i<10;++i){const auto a=address_+0x28+i*0x30,b=a+10*0x30;std::array<std::uint8_t,0x30> saved{};
        for(unsigned k=0;k<saved.size();++k)saved[k]=memory_.byte(a+k);
        memory_.copy(a,b,saved.size());for(unsigned k=0;k<saved.size();++k)memory_.byte(b+k,saved[k]);}
    const auto flags=memory_.word(address_+0x18);memory_.word(address_+0x18,(flags&~2u)|(((flags>>1)^1u)&1u)<<1);
}
// Debug001BF650, E-0099-W14.
void TrackRoute::ReverseTrafficRoute(float track_length){
    for(int i=0;i<memory_.signed_word(address_+0x10);++i)TrackRoutePoint(memory_,address_+0x428+std::uint32_t(i)*0x70).ReverseTrafficRoutePoint(track_length);
}
// Debug001BF6B0, E-0099-W14. Original excludes first point from extrema after
// using it as the previous-distance anchor. Listing confirms MAX.S/MIN.S at wrap.
void TrackRoute::SetFirstAndLast(const port::track::RouteData& data){
    const auto count=memory_.signed_word(address_+0x10);
    if(count<1)hp2::not_implemented("0x001BF6B0","original route extrema require a first point");
    const float length=data.paths.track_length,half=length*0.5f,tolerance=memory_.scalar(0x00325da0);
    float extrema[2][2]{{length,0},{length,0}};unsigned segment=0;bool wrapped=false;
    TrackPathCoordinate coordinate;coordinate.UpdatePosition(data.paths,{{memory_.scalar(address_+0x428),memory_.scalar(address_+0x42c)}},tolerance);
    float previous=coordinate.DistanceAroundTrack;
    for(int i=1;i<count;++i){const auto point=address_+0x428+std::uint32_t(i)*0x70;
        coordinate.UpdatePosition(data.paths,{{memory_.scalar(point),memory_.scalar(point+4)}},tolerance);
        const float distance=coordinate.DistanceAroundTrack;
        if(half<std::abs(distance-previous)){wrapped=true;segment^=1;}
        if(extrema[segment][1]<distance)extrema[segment][1]=distance;
        if(distance<extrema[segment][0])extrema[segment][0]=distance;
        previous=distance;
    }
    memory_.scalar(address_+0x20,wrapped?std::max(extrema[0][0],extrema[1][0]):extrema[0][0]);
    memory_.scalar(address_+0x24,wrapped?std::min(extrema[0][1],extrema[1][1]):extrema[0][1]);
}
// Debug001BE128, E-0099-W14. Only targets on racing routes(<50) change.
void Signpost::ReverseSigns(Memory& memory,const port::track::RouteData& data){
    for(unsigned i=0;i<data.signpost_count;++i){const auto sign=data.signposts+i*8;const unsigned route=memory.byte(sign);
        if(route<50){if(!data.routes[route])throw std::out_of_range("Original reverse sign target route missing");
            const auto count=memory.word(data.routes[route]+0x10);
            memory.half(sign+2,std::uint16_t(count-memory.half(sign+2)-1));
            const auto flags=memory.word(sign);memory.word(sign,(flags&~0x800u)|((1u-((flags>>11)&1u))&1u)<<11);}}
}
// Debug001BE1F0, E-0099-W14. Distance follows sign target -> its sign -> second
// target. It is not simply the first target position; one coordinate is reused.
void Signpost::SetDistances(Memory& memory,const port::track::RouteData& data){
    TrackPathCoordinate coordinate;const auto tolerance=memory.scalar(0x00325da0);
    for(unsigned i=0;i<data.signpost_count;++i){const auto sign=data.signposts+i*8;
        const auto first=port::track::checked_route_point(data,memory,memory.byte(sign),memory.half(sign+2));
        const auto next=port::track::checked_sign(data,memory,first);
        const auto second=port::track::checked_route_point(data,memory,memory.byte(next),memory.half(next+2));
        coordinate.UpdatePosition(data.paths,{{memory.scalar(second),memory.scalar(second+4)}},tolerance);
        memory.scalar(sign+4,coordinate.DistanceAroundTrack);
    }
}
// Debug001C0098, E-0099-W14. Exact one-based heap traversal over zero-based
// storage, strict comparator branches and tie permutation, not std::sort.
void HeapSort(Memory& memory,std::uint32_t buffer,int num,int size,const std::function<float(std::uint32_t,std::uint32_t)>& compare){
    if(num<2)return;
    if(size<=0||!compare)throw std::invalid_argument("Original HeapSort requires a positive item size and comparator");
    Memory::Scratch scratch(memory);const auto temporary=memory.allocate(std::uint32_t(size));
    int end=num-1,level=num/2+1;bool extraction=level<2;
    auto current=buffer+std::uint32_t((level-1)*size);
    for(;;){
        if(extraction){const auto tail=buffer+std::uint32_t(end*size);memory.copy(temporary,tail,size);memory.copy(tail,buffer,size);
            const bool done=end==1;num=end;--end;if(done){memory.copy(buffer,temporary,size);return;}}
        else{--level;current-=std::uint32_t(size);memory.copy(temporary,current,size);extraction=level<2;}
        int child=level*2,hole=level-1;
        while(child<=num){int selected=child-1;
            if(child<num&&compare(buffer+std::uint32_t((child-1)*size),buffer+std::uint32_t(child*size))<0){++child;selected=child-1;}
            const auto source=buffer+std::uint32_t(selected*size);
            if(compare(temporary,source)<0){memory.copy(buffer+std::uint32_t(hole*size),source,size);hole=child-1;child*=2;}
            else child=num+1;
        }
        memory.copy(buffer+std::uint32_t(hole*size),temporary,size);
    }
}
// Debug001C02D8, E-0099-W14. Compare the pointed-to Distance fields, not indices.
float CompareSignposts(Memory& memory,std::uint32_t before,std::uint32_t after){return memory.scalar(memory.word(before)+4)-memory.scalar(memory.word(after)+4);}
// Debug001C02F0, E-0099-W14. Whole actual postload preparation after the
// explicit Retail-to-Debug schema adapter; original directions/order retained.
void InitTrackRoutes(Memory& memory,port::track::RouteData& data,bool forwards){
    data.paths.reverse_direction=!forwards;
    if(!forwards){for(unsigned r=0;r<70;++r)if(data.routes[r]){
            TrackRoute route(memory,data.routes[r]);if(r>=50)route.ReverseTrafficRoute(data.paths.track_length);else route.ReverseRoute(data.paths.track_length);}
        Signpost::ReverseSigns(memory,data);}
    for(unsigned i=0;i<data.signpost_count;++i){const auto sign=data.signposts+i*8;
        if((memory.word(sign)&0x700)==0x500){const auto r=memory.byte(sign);
            if(!data.routes[r])throw std::out_of_range("Original DontTake sign target route missing");
            memory.word(data.routes[r]+0x18,memory.word(data.routes[r]+0x18)|4);}}
    for(auto route:data.routes)if(route)TrackRoute(memory,route).SetFirstAndLast(data);
    Signpost::SetDistances(memory,data);
    Memory::Scratch scratch(memory);const auto count=std::uint32_t(data.signpost_count);
    // Zero-size host allocations have no logical address. The original zero
    // count performs no sort, remap or copy; retain those empty operations.
    if(count==0){data.initialized=true;return;}
    const auto rows=memory.allocate(count*8);
    for(unsigned i=0;i<count;++i){memory.word(rows+i*8,data.signposts+i*8);memory.word(rows+i*8+4,i);}
    HeapSort(memory,rows,int(count),8,[&](auto before,auto after){return CompareSignposts(memory,before,after);});
    std::array<std::uint8_t,256> new_indices{};
    std::vector<std::uint32_t> metadata_order;
    if(!data.retail_signpost_order.empty())metadata_order.resize(count);
    for(unsigned i=0;i<count;++i){const auto old=memory.word(rows+i*8+4);new_indices.at(old)=std::uint8_t(i);
        if(!metadata_order.empty())metadata_order[i]=data.retail_signpost_order.at(old);}
    for(auto route:data.routes)if(route)for(int i=0;i<memory.signed_word(route+0x10);++i){const auto point=route+0x428+std::uint32_t(i)*0x70;const auto index=memory.byte(point+0x2c);
        if(index!=0xff){if(index>=count)throw std::out_of_range("Original remapped sign index outside array");memory.byte(point+0x2c,new_indices[index]);}}
    const auto sorted=memory.allocate(count*8);
    for(unsigned i=0;i<count;++i){const auto p=sorted+i*8;memory.byte(p,0);memory.word(p,memory.word(p)&0xffffc0ff);memory.half(p+2,0);memory.word(p,memory.word(p)&0xffffbfff);}
    for(unsigned i=0;i<count;++i)memory.copy(sorted+i*8,memory.word(rows+i*8),8);
    memory.copy(data.signposts,sorted,count*8);
    if(!metadata_order.empty())data.retail_signpost_order=std::move(metadata_order);
    data.initialized=true;
}
}

namespace hp2::debug {
// Debug001BE280; E-0099-W24; original trackroute.cpp.
int Signpost::GetIndex(::port::driving::Memory& m,const ::port::track::RouteData& routes,float dat){
 if(!routes.initialized||!routes.signposts||!routes.signpost_count)
  hp2::not_implemented("0x001BE280","Signpost::GetIndex requires the original nonempty loaded signpost array");
 for(std::size_t i=0;i<routes.signpost_count;++i)if(dat<=m.scalar(routes.signposts+static_cast<std::uint32_t>(i)*8+4))return static_cast<int>(i);
 return 0;
}
}

namespace hp2::debug {
// Debug001BE4B8, E-0099-W37. Original two vector scale-adds. The three
// uninitialized temporary pad words cannot be recovered from the ELF; the
// resulting host padding word is an explicit port operand, not original data.
void TrackRoutePoint::GetPosition(std::uint32_t out,float d,float l,std::uint32_t lane)const{const auto a=address_;auto& m=memory_;const float dx=m.scalar(a+0xc),dy=m.scalar(a+0x10);const float x=dy*l+m.scalar(a),y=(-dx)*l+m.scalar(a+4),z=m.scalar(a+0x20)*l+m.scalar(a+8);m.scalar(out,x);m.scalar(out+4,y);m.scalar(out+8,z);m.word(out+12,lane);m.scalar(out,dx*d+x);m.scalar(out+4,dy*d+y);m.scalar(out+8,m.scalar(a+0x24)*d+z);}
}

namespace hp2::debug {
// Debug001BF830; E-0099-W37. Original inclusive wrap interval; equal endpoints
// deliberately accept all distances rather than inventing an empty interval.
bool TrackRoute::DistanceIsWithinRoute(float distance)const{const float begin=memory_.scalar(address_+0x20),end=memory_.scalar(address_+0x24);if(begin<end)return begin<=distance&&distance<=end;return begin<=distance||distance<=end;}
}

namespace hp2::debug {
// Debug001BEDE0; E-0099-W47. Original increments the index before testing
// the final bound: even a forward-facing last point returns null.
std::uint32_t TrackRoute::FindClosestPointAhead(const std::array<float,2>& position,std::uint32_t initial)const{if(!initial)return 0;auto& m=memory_;int index=static_cast<std::int32_t>(initial-(address_+0x428))/0x70;std::uint32_t point=address_+0x428+0x70*index;do{const auto current=point;++index;const float projection=-((position[0]-m.scalar(current))*m.scalar(current+0xc)+(position[1]-m.scalar(current+4))*m.scalar(current+0x10));if(0.f<=projection)return index<m.signed_word(address_+0x10)?current:0;point=current+0x70;}while(index<m.signed_word(address_+0x10));return 0;}
// Debug001BE5A8/001BE0C0/001BE0E8; E-0099-W47. Actual shared globals.
std::uint32_t TrackRoutePoint::GetSignpost()const{return memory_.word(0x328190)+8*memory_.byte(address_+0x2c);}
std::uint32_t Signpost::GetNewRoute(::port::driving::Memory& m,std::uint32_t sign){return m.word(0x30bb40+4*m.byte(sign));}
std::uint32_t Signpost::GetNewRoutePoint(::port::driving::Memory& m,std::uint32_t sign){return GetNewRoute(m,sign)+0x428+0x70*m.half(sign+2);}
}

#include "game_core/indep/inc/Replay.hpp"
namespace hp2::debug {
// Debug001BFBE0; E-0099-W74; replay-frame/001bfbe0.json.
void TrackRouteManager::DoPointerSnapshot(std::uint32_t snapshot,::port::race::SnapshotRoutePointer pointer){
 ::port::driving::Memory::Temporary local(m_,4);std::int32_t number=-1;
 if(!m_.byte(snapshot+0x3d)&&m_.word(pointer.address))number=m_.signed_half(m_.word(pointer.address)+0xa);
 m_.word(local.address(),number);ReplaySnapshot(m_,snapshot).MoveData(local.address(),1,4,1);
 if(m_.byte(snapshot+0x3d)){number=m_.signed_word(local.address());if(number>=100)hp2::not_implemented("0x001BFBE0","Restored route outside original100-slot manager");m_.word(pointer.address,number<0?0:m_.word(address_+0x10+4*number));}
}
// Debug001BFC78; E-0099-W74. Exact inverse multiply/shift is retained for source point index.
void TrackRouteManager::DoPointerSnapshot(std::uint32_t snapshot,::port::race::SnapshotRoutePointPointer pointer,std::uint32_t route){
 ::port::driving::Memory::Temporary local(m_,4);std::int32_t number=-1;const auto point=m_.word(pointer.address);
 if(point&&route){const std::uint32_t product=(point-route-0x428)*0xb6db6db7u;number=static_cast<std::int32_t>(product)>>4;}
 m_.word(local.address(),number);ReplaySnapshot(m_,snapshot).MoveData(local.address(),2,4,1);number=m_.signed_word(local.address());
 if((m_.byte(snapshot+0x3c)||m_.byte(snapshot+0x3d))&&m_.word(pointer.address)&&number==-1)m_.word(pointer.address,0);
 if(m_.byte(snapshot+0x3d)){if(number>=0&&!route)hp2::not_implemented("0x001BFC78","Nonnegative restored point with null route");m_.word(pointer.address,number<0?0:route+0x428+0x70*static_cast<std::uint32_t>(number));}
}
// Debug001BFD50; E-0099-W74; replay-frame/001bfd50.json. Full point code is four bytes.
void TrackRouteManager::DoPointerSnapshot(std::uint32_t snapshot,::port::race::SnapshotRoutePointPointer pointer){
 ::port::driving::Memory::Temporary local(m_,4);std::uint32_t code=0xffffffffu;
 if(!m_.byte(snapshot+0x3d)){
  if(const auto point=m_.word(pointer.address)){
   // TrackRoutePoint.RouteNumber byte is confirmed at+0x6F by original type.
   const auto number=m_.byte(point+0x6f);if(number>=100)hp2::not_implemented("0x001BFD50","Point route number outside manager");
   const auto route=m_.word(address_+0x10+4*number);if(!route)hp2::not_implemented("0x001BFD50","Snapshot point references missing original route");
   const auto index=static_cast<std::int32_t>((point-route-0x428)*0xb6db6db7u)>>4;
   code=static_cast<std::uint32_t>(index)+static_cast<std::uint32_t>(m_.signed_half(route+0xa))*0x10000u;
  }
 }else code=0;
 m_.word(local.address(),code);ReplaySnapshot(m_,snapshot).MoveData(local.address(),4,4,1);
 if(m_.byte(snapshot+0x3d)){code=m_.word(local.address());if(code==0xffffffffu)m_.word(pointer.address,0);else{
  const auto number=static_cast<std::int32_t>(code)>>16;if(number<0||number>=100)hp2::not_implemented("0x001BFD50","Restored route code outside original manager");
  const auto route=m_.word(address_+0x10+4*number);if(!route)hp2::not_implemented("0x001BFD50","Restored point references null route");
  m_.word(pointer.address,route+0x428+0x70*(code&0xffff));
 }}
}
}
