
// TASK-0042: bounded host projections; original member ABI remains unverified.
#include "game_core/port/driving/projection_support.hpp"
// Retail 0x00152070; E-0042-C021; E-0042-S001; exact Debug TU ..\indep\src\track.cpp
namespace port::driving {
float PlaneContact::GetElevation(Memory& m,std::uint32_t,std::uint32_t point,
    std::uint32_t material,std::uint32_t plane,std::uint32_t found) {
    const auto p=m.vector(point);
    for(float v:p)if(!std::isfinite(v))throw std::invalid_argument("Nonfinite synthetic plane query");
    if(material)m.word(material,static_cast<std::uint32_t>(surface_material_at(settings,p[0])));
    if(plane)m.vector(plane,{{0,0,1,height}});
    if(found)m.word(found,1);
    return height;
}
}

#include "game_core/indep/src/track.hpp"

namespace hp2::debug {
// TASK-0099 E-0099-W72. Actual shared table pointer and highest-number domain.
std::uint32_t GetTrackInfo(const ::port::driving::Memory& m,int number){if(number<=0||number>m.signed_word(0x3273a0))return 0;const auto row=m.word(0x32739c)+std::uint32_t(number-1)*0xa0;return m.byte(row)?row:0;}
}

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x00152070; E-0042-C021; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/00152070.md.
void TopologyCoordinate::GetElevation() {
    hp2::not_implemented("0x00152070", "TopologyCoordinate::GetElevation (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x00152B10; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00152b10.md.
void TopologyTree::Sub_00152B10() {
    hp2::not_implemented("0x00152B10", "TopologyTree::Sub_00152B10 (signature and full body unresolved)");
}

// ELF 0x00152C10; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00152c10.md.
void TopologyTree::Loader() {
    hp2::not_implemented("0x00152C10", "TopologyTree::Loader (signature and full body unresolved)");
}

// ELF 0x00152D30; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00152d30.md.
void TopologyTree::Unloader() {
    hp2::not_implemented("0x00152D30", "TopologyTree::Unloader (signature and full body unresolved)");
}

// ELF 0x00152DB0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00152db0.md.
void TopologyTree::GetSquareCorner() {
    hp2::not_implemented("0x00152DB0", "TopologyTree::GetSquareCorner (signature and full body unresolved)");
}

// ELF 0x00152E08; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00152e08.md.
void TopologyTree::GetSquare() {
    hp2::not_implemented("0x00152E08", "TopologyTree::GetSquare (signature and full body unresolved)");
}

// ELF 0x00152F38; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00152f38.md.
void TopologyTree::GetIntersectingPolygons() {
    hp2::not_implemented("0x00152F38", "TopologyTree::GetIntersectingPolygons (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001517D8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001517d8.md.
void TrackPolygon::PointInShaft() {
    hp2::not_implemented("0x001517D8", "TrackPolygon::PointInShaft (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered { using namespace port::driving; }

#include "game_core/src/bMath.hpp"
#include "game_core/indep/inc/Table.hpp"
#include "game_core/indep/src/table.hpp"

// Debug 0x001530A8; E-0044-M02; source ownership E-0042-S001.
namespace hp2::recovered {
float GetRoadNoiseGridValue(Memory& m,std::int32_t x,std::int32_t y) {
    const auto a=static_cast<std::uint32_t>(x),b=static_cast<std::uint32_t>(y);
    const auto index=(a*b*11u+(a+1u)*(b+1u)*13u)&15u;
    return m.scalar(0x002f0d40+index*4);
}
}
// Debug 0x001530E8; E-0044-M02; source ownership E-0042-S001.
namespace hp2::recovered {
float GetRoadNoise(Memory& m,std::uint32_t point,std::int32_t material) {
    if(material<0||material>=32)throw std::out_of_range("Material outside bounded road projection");
    const auto row=0x003102e8u+static_cast<std::uint32_t>(material)*0x90u;
    float x=m.scalar(point)*m.scalar(row+0x18),y=m.scalar(point+4)*m.scalar(row+0x18);
    float fx=static_cast<float>(cvt_w_s(x)),fy=static_cast<float>(cvt_w_s(y));
    if(x<fx)fx=fx-1.0f;
    if(y<fy)fy=fy-1.0f;
    const auto ix=cvt_w_s(fx),iy=cvt_w_s(fy);
    y=y-fy;x=x-fx;const float inverse=1.0f-x;
    float value=inverse*(1.0f-y);value=value*hp2::recovered::GetRoadNoiseGridValue(m,ix,iy);
    float term=x*(1.0f-y);term=term*hp2::recovered::GetRoadNoiseGridValue(m,ix+1,iy);value=value+term;
    term=inverse*y;term=term*hp2::recovered::GetRoadNoiseGridValue(m,ix,iy+1);value=value+term;
    term=x*y;term=term*hp2::recovered::GetRoadNoiseGridValue(m,ix+1,iy+1);value=value+term;
    return value*m.scalar(row+0x1c);
}
}
// TASK-0048 E-0048-T01: Retail SLUS_203.62 SHA-256
// 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
#include "game_core/port/assets/elab_chunks.hpp"
#include <algorithm>
#include <limits>
namespace hp2::debug {
// Debug001521A8, E-0099-W17. This wraps the previously recovered fresh polygon
// query of00151CC0/Retail00152070; it is not a nearest-XY or terrain shortcut.
bool TopologyCoordinate::HasTopology(const ::port::driving::Memory& memory,const ::port::track::Topology& topology,const std::array<float,2>& position){
    return hp2::recovered::TopologyCoordinate::GetElevation(topology,{{position[0],position[1],memory.scalar(0x00324f68)}}).found;
}
}
namespace {
std::uint16_t track_u16(const std::uint8_t* p){return std::uint16_t(p[0])|(std::uint16_t(p[1])<<8);}
std::int16_t track_i16(const std::uint8_t* p){const auto v=track_u16(p);return static_cast<std::int16_t>(v);}
std::array<float,3> track_cross(const std::array<float,3>& a,const std::array<float,3>& b){return {{a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]}};}
float track_dot(const std::array<float,3>& a,const std::array<float,3>& b){return (a[0]*b[0]+a[1]*b[1])+a[2]*b[2];}
float track_inside(const port::track::Polygon& p,const std::array<float,3>& q){
    const auto edge=[&](unsigned a,unsigned b){return (p.vertices[b][1]-p.vertices[a][1])*(q[0]-p.vertices[a][0])-(p.vertices[b][0]-p.vertices[a][0])*(q[1]-p.vertices[a][1]);};
    float value=std::max(-1.0f,edge(0,1));
    if(value<=0)value=std::max(value,edge(1,2));
    // The original always reads vertex 3 here, including triangle records.
    if(value<=0)value=std::max(value,edge(3,0));
    if(value<=0&&p.count==4)value=std::max(value,edge(2,3));
    return value;
}
}
namespace hp2::recovered {
// Retail 0x001514A8; E-0048-T01; original packed-to-expanded conversion.
port::track::Polygon TrackPolygon::Sub_001514A8(const std::uint8_t* bytes){
    port::track::Polygon p;p.flags=bytes[3];p.terrain=static_cast<std::int8_t>(bytes[2]);p.count=(p.flags&0x10)?4:3;
    const auto selector=bytes[1];
    p.bounds={{track_i16(bytes+8+(selector&3)*2)*0.125f,track_i16(bytes+16+((selector>>2)&3)*2)*0.125f,
        track_i16(bytes+8+((selector>>4)&3)*2)*0.125f,track_i16(bytes+16+(selector>>6)*2)*0.125f}};
    for(unsigned i=0;i<4;++i){p.vertices[i]={{track_i16(bytes+8+i*2)*0.125f,track_i16(bytes+16+i*2)*0.125f,
        static_cast<float>(track_i16(bytes+4))+track_i16(bytes+24+i*2)*(1.0f/256.0f)}};if(p.flags&4)p.vertices[i][2]*=4;}
    std::array<float,3> a{},b{};for(unsigned j=0;j<3;++j){a[j]=p.vertices[1][j]-p.vertices[0][j];b[j]=p.vertices[2][j]-p.vertices[0][j];}
    auto n=track_cross(a,b);const auto length=std::sqrt(track_dot(n,n));
    if(!(length>0)||!std::isfinite(length))throw std::invalid_argument("Degenerate track topology polygon");
    for(unsigned j=0;j<3;++j){n[j]/=length;p.plane[j]=n[j];}p.plane[3]=track_dot(p.vertices[0],n);return p;
}
// Retail 0x001517D8; E-0048-T01. Original plane-shaft edge tests.
bool TrackPolygon::PointInShaft(const port::track::Polygon& p,const std::array<float,3>& point,float* distance){
    std::array<std::array<float,3>,4> d{};for(unsigned i=0;i<p.count;++i)for(unsigned j=0;j<3;++j)d[i][j]=p.vertices[i][j]-point[j];
    const std::array<float,3> normal{{p.plane[0],p.plane[1],p.plane[2]}};
    for(unsigned i=0;i<p.count;++i)if(track_dot(track_cross(d[i],d[(i+1)%p.count]),normal)<0)return false;
    if(distance)*distance=-track_dot(d[0],normal);
    return true;
}
// Retail 0x00152DB0; E-0048-T01. Fixed-point 32-unit cell corner.
float TopologyTree::GetSquareCorner(float v){
    if(!std::isfinite(v)||v<=-32768||v>=32768)throw std::out_of_range("Track square coordinate outside original fixed point range");
    const auto fixed=static_cast<std::int32_t>(v*65536.0f);
    return static_cast<float>(static_cast<std::int32_t>(static_cast<std::uint32_t>(fixed)&0xffe00000u))*(1.0f/65536.0f);
}
// Retail 0x00152C10; E-0048-T01. Port vectors replace original pointers.
bool TopologyTree::Loader(port::track::Topology& t,std::uint32_t tag,const std::vector<std::uint8_t>& data,std::size_t header,std::size_t size){
    if(tag<0x34130||tag>0x34133)return false;
    if(header>data.size()||data.size()-header<8||size>data.size()-header-8)throw std::invalid_argument("Truncated topology chunk");
    std::size_t start=header+8,end=start+size;if(tag==0x34130||tag==0x34131)start=(header+0x17)&~std::size_t(15);
    const std::size_t stride=tag==0x34130?8:tag==0x34131?12:tag==0x34132?32:2;
    if(start>end||(end-start)%stride)throw std::invalid_argument("Misaligned topology chunk records");
    if(tag==0x34130){if(!t.nodes.empty())throw std::invalid_argument("Duplicate topology nodes");for(;start<end;start+=8){std::array<std::uint16_t,4> r{};for(unsigned j=0;j<4;++j)r[j]=track_u16(&data[start+j*2]);t.nodes.push_back(r);}}
    if(tag==0x34131){if(!t.squares.empty())throw std::invalid_argument("Duplicate topology squares");for(;start<end;start+=12){std::array<std::uint16_t,6> r{};for(unsigned j=0;j<6;++j)r[j]=track_u16(&data[start+j*2]);t.squares.push_back(r);}}
    if(tag==0x34132){if(!t.polygons.empty())throw std::invalid_argument("Duplicate topology polygons");for(;start<end;start+=32)t.polygons.push_back(TrackPolygon::Sub_001514A8(&data[start]));}
    if(tag==0x34133){if(!t.indices.empty())throw std::invalid_argument("Duplicate topology indices");for(;start<end;start+=2)t.indices.push_back(track_u16(&data[start]));}return true;
}
// Retail 0x00152D30; E-0048-T01.
bool TopologyTree::Unloader(port::track::Topology& t,std::uint32_t tag){switch(tag){case 0x34130:t.nodes.clear();return true;case 0x34131:t.squares.clear();return true;case 0x34132:t.polygons.clear();return true;case 0x34133:t.indices.clear();return true;default:return false;}}
// Retail 0x00152E08; E-0048-T01. Original missing-child mode 0 only.
std::optional<std::size_t> TopologyTree::GetSquare(const port::track::Topology& t,float x,float y){
    float corner_x=-16384,corner_y=-16384;std::size_t node=0;
    for(int level=10;level>0;--level){if(node>=t.nodes.size())throw std::out_of_range("Topology node reference outside chunk");const float half=static_cast<float>(1<<level)*32*0.5f;
        unsigned q=0;if(corner_x+half<=x){q|=1;corner_x+=half;}if(corner_y+half<=y){q|=2;corner_y+=half;}
        const auto child=t.nodes[node][q];if(!child)return {};if(level==1){if(child>=t.squares.size())throw std::out_of_range("Topology square reference outside chunk");return child;}node=child;
    }return {};
}
// Retail 0x00152070; E-0048-T01. Fresh candidate branch, before persistent cache optimization.
port::track::Contact TopologyCoordinate::GetElevation(const port::track::Topology& t,const std::array<float,3>& q){
    for(float f:q)if(!std::isfinite(f))throw std::invalid_argument("Nonfinite track elevation query");
    auto candidates=t.intersecting({{q[0]-8.5f,q[1]-8.5f}},{{q[0]+8.5f,q[1]+8.5f}});
    port::track::Contact result;result.height=q[2];
    for(int pass=0;pass<2;++pass){float best=-std::numeric_limits<float>::max();
        for(auto index:candidates){const auto& p=t.polygons[index];if(p.flags&(2|8))continue;
            if(q[0]<p.bounds[0]||q[0]>p.bounds[2]||q[1]<p.bounds[1]||q[1]>p.bounds[3])continue;
            const auto inside=track_inside(p,q);if(inside>0)continue;
            const float height=(p.plane[3]-(p.plane[0]*q[0]+p.plane[1]*q[1]))/p.plane[2];
            if(best<height&&(height<(pass?std::numeric_limits<float>::max():q[2]+1.0f))){best=height;result={true,height,p.plane,p.terrain,index};if(!(p.flags&1)&&inside<0)return result;}
        }if(result.found)return result;
    }return result;
}
}
namespace port::track {
void Topology::clear(){nodes.clear();squares.clear();polygons.clear();indices.clear();}
void Topology::load(const std::vector<std::uint8_t>& data){
    Topology pending;std::size_t consumed=0;const auto chunks=hp2::assets::parse_chunks_top(data,&consumed);
    if(consumed!=data.size())throw std::invalid_argument("Incomplete track bundle chunk stream");
    const auto visit=[&](const auto& self,const std::vector<hp2::assets::Chunk>& items)->void{for(const auto& c:items){
        hp2::recovered::TopologyTree::Loader(pending,c.tag,data,c.header_offset,c.size);
        if(c.is_container()){
            std::size_t child_consumed=0;
            hp2::assets::parse_chunks(data.data(),data.size(),c.payload_offset(),c.payload_offset()+c.size,&child_consumed);
            if(child_consumed!=c.payload_offset()+c.size)throw std::invalid_argument("Incomplete nested track chunk stream");
            self(self,c.children);
        }
    }};visit(visit,chunks);
    if(!pending.ready())throw std::invalid_argument("Track bundle has no complete topology tree");
    for(auto i:pending.indices)if(i>=pending.polygons.size())throw std::out_of_range("Topology polygon index outside chunk");
    for(const auto& s:pending.squares){const auto count=static_cast<std::int16_t>(s[5]);if(count<0||std::size_t(s[4])+count>pending.indices.size())throw std::out_of_range("Topology square range outside index chunk");}
    std::vector<bool> visited(pending.nodes.size()*10);
    const auto validate_nodes=[&](const auto& self,std::size_t node,unsigned depth)->void{
        if(node>=pending.nodes.size())throw std::out_of_range("Topology node reference outside chunk");
        const auto key=node*10+depth-1;if(visited[key])return;visited[key]=true;
        for(auto child:pending.nodes[node])if(child){
            if(depth==1){if(child>=pending.squares.size())throw std::out_of_range("Topology square reference outside chunk");}
            else self(self,child,depth-1);
        }
    };validate_nodes(validate_nodes,0,10);
    *this=std::move(pending);
}
} // namespace port::track
namespace hp2::recovered {
// Retail 0x001531F8; E-0048-T01; no PS2 allocator/cache ABI claim.
std::vector<std::size_t> TopologyTree::BuildTrackPolygons(const port::track::Topology& t,const std::array<float,2>& low,const std::array<float,2>& high,std::size_t capacity) {
    const auto& polygons=t.polygons;const auto& squares=t.squares;const auto& indices=t.indices;
    if(!t.ready())throw std::logic_error("Track topology is not loaded");
    for(unsigned j=0;j<2;++j)if(!std::isfinite(low[j])||!std::isfinite(high[j])||low[j]>high[j])throw std::invalid_argument("Invalid track query bounds");
    std::vector<std::size_t> out;std::vector<bool> seen(polygons.size());
    // Retail 0x001531F8 uses the strict fixed-point domain and a capacity of 160.
    if(low[0]<=-32768||low[1]<=-32768||high[0]>=32767||high[1]>=32767)return out;
    for(float y=hp2::recovered::TopologyTree::GetSquareCorner(low[1]);y<=high[1];y+=32)
        for(float x=hp2::recovered::TopologyTree::GetSquareCorner(low[0]);x<=high[0];x+=32){
            const auto cell=hp2::recovered::TopologyTree::GetSquare(t,x,y);if(!cell)continue;const auto& s=squares[*cell];
            for(std::size_t j=s[4];j<std::size_t(s[4])+s[5];++j){const auto index=indices.at(j);const auto& p=polygons.at(index);
                if(!seen[index]&&p.bounds[0]<=high[0]&&p.bounds[1]<=high[1]&&low[0]<=p.bounds[2]&&low[1]<=p.bounds[3]&&out.size()<capacity){seen[index]=true;out.push_back(index);}}
        }return out;
}
} // namespace hp2::recovered
namespace port::track {
std::vector<std::size_t> Topology::intersecting(const std::array<float,2>& low,const std::array<float,2>& high,std::size_t capacity) const {return hp2::recovered::TopologyTree::BuildTrackPolygons(*this,low,high,capacity);}
Contact Topology::elevation(const std::array<float,3>& q)const{return hp2::recovered::TopologyCoordinate::GetElevation(*this,q);}
}

namespace hp2::recovered {
// Retail 0x00151990; Debug 0x001515E0; E-0048-T02.
bool TrackPolygon::SegmentIntersects(const port::track::Polygon& p,const std::array<float,3>& first,const std::array<float,3>& second,std::array<float,3>* intersection){
    const std::array<float,3> n{{p.plane[0],p.plane[1],p.plane[2]}};
    std::array<float,3> v1{},v2{};for(unsigned j=0;j<3;++j){v1[j]=first[j]-p.vertices[0][j];v2[j]=second[j]-p.vertices[0][j];}
    const auto d1=track_dot(v1,n),d2=track_dot(v2,n);if(!(d1*d2<0))return false;
    const auto fraction=std::abs(d1)/(std::abs(d1)+std::abs(d2));std::array<float,3> point{};
    for(unsigned j=0;j<3;++j)point[j]=first[j]+(second[j]-first[j])*fraction;
    if(!PointInShaft(p,point,nullptr))return false;
    if(intersection)*intersection=point;
    return true;
}
}

#include "game_core/indep/inc/CollisionBody.hpp"
namespace port::track {
std::vector<std::size_t> Topology::collision_candidates(const CollisionCylinder& body) const {
    for(float v:body.center)if(!std::isfinite(v))throw std::invalid_argument("Nonfinite collision cylinder center");
    for(float v:body.axis)if(!std::isfinite(v))throw std::invalid_argument("Nonfinite collision cylinder axis");
    if(!std::isfinite(body.radius)||!std::isfinite(body.base_height)||!std::isfinite(body.top_height)||body.radius<0||body.base_height<0||body.top_height<0)
        throw std::invalid_argument("Invalid collision cylinder dimensions");
    // The caller publishes dimensions from recovered body data. This host helper owns no body IDs or witnesses.
    const float extent=body.radius+std::max(body.base_height,body.top_height);
    const auto candidates=intersecting({{body.center[0]-extent,body.center[1]-extent}},{{body.center[0]+extent,body.center[1]+extent}});
    std::vector<std::size_t> out;
    for(auto index:candidates)if(hp2::recovered::DynamicCollisionBody::TrackPolygonCheck(body,polygons[index])&&out.size()<48)out.push_back(index);
    std::sort(out.begin(),out.end());return out;
}
}

#include "game_core/inc/bSlotPool.hpp"
#include "game_core/port/driving/projections.hpp"
#include <cstring>
namespace hp2::debug {
// Debug00152760; E-0099-W26.
void TopologyTree::Sub_00152760(){
 for(unsigned i=0;i<0x24;i+=4)m_.word(address_+i,0);
 m_.word(address_+0x24,address_+0x24);m_.word(address_+0x28,address_+0x24);
}
// Debug00152798; E-0099-W26. Explicit original tree startup, not a lazy query side effect.
void TopologyTree::InitTree(){
 const auto pool=bNewSlotPool(m_,0x80,0x400,0x002a8108,0x10);m_.word(0x00327394,pool);
 m_.word(0x00327380,0);m_.word(0x00327384,0);m_.word(pool+0x14,m_.word(pool+0x14)&~3u);
}
// Debug00152E38; E-0099-W26. Original byte reference count wraps.
void TopologyTree::UnAssignTrackPolygon(std::uint32_t polygon){m_.byte(polygon+8,static_cast<std::uint8_t>(m_.byte(polygon+8)-1));}
// Debug00153018; E-0099-W26.
void TopologyTree::RemoveTrackPolygons(std::uint32_t table,int count){for(int i=0;i<count;++i)UnAssignTrackPolygon(m_.word(table+static_cast<unsigned>(i)*4));}
// Debug00152CF8; E-0099-W26. The original stops after101 deletions.
void TopologyTree::FlushUnusedTrackPolygons(){
 const auto head=address_+0x24;auto node=m_.word(head);int removed=0;
 while(node!=head){const auto next=m_.word(node);if(!m_.byte(node+8)){
  const auto previous=m_.word(node+4);m_.word(previous,next);m_.word(next+4,previous);
  const auto source=m_.word(node+12);if(source)m_.half(source+6,0xffff);
  SlotPool(m_,m_.word(0x00327394)).Free(node);if(++removed>100)break;node=m_.word(previous);
 }else node=next;}
 if(!removed)hp2::not_implemented("0x00152CF8","original topology pool exhaustion invokes bBreak with no unused polygons");
}
// Debug00152D98 ->001510F8; E-0099-W26. Source records, slot numbers and list identities are shared.
std::uint32_t TopologyTree::AssignTrackPolygon(std::uint32_t source){
 auto& m=m_;const auto pool=m.word(0x00327394);if(!pool)hp2::not_implemented("0x00152D98","original TrackPolygonSlotPool was not initialized");
 auto number=m.signed_half(source+6);std::uint32_t polygon;
 if(number<0){if(!m.word(pool+0x10))FlushUnusedTrackPolygons();polygon=SlotPool(m,pool).Malloc();
  if(!polygon)hp2::not_implemented("0x00152D98","original TrackPolygon pool allocation failed");
  // Debug001510F8. Preserve fixed-point integer wrapping and VU reciprocal
  // normalization grouping; the older decoded Retail projection divides lanes.
  ::port::track::Polygon expanded;const auto bits=m.byte(source+1);
  expanded.bounds={{m.signed_half(source+8+(bits&3)*2)*0.125f,m.signed_half(source+16+((bits>>2)&3)*2)*0.125f,
   m.signed_half(source+8+((bits>>4)&3)*2)*0.125f,m.signed_half(source+16+(bits>>6)*2)*0.125f}};
  expanded.count=(m.byte(source+3)&0x10)?4:3;
  for(unsigned i=0;i<4;++i){const auto fixed=static_cast<std::uint32_t>(static_cast<std::int32_t>(m.signed_half(source+24+i*2)))*256u+
    static_cast<std::uint32_t>(static_cast<std::int32_t>(m.signed_half(source+4)))*65536u;
   float z=static_cast<float>(static_cast<std::int32_t>(fixed))*0x1p-16f;if(m.byte(source+3)&4)z*=4;
   expanded.vertices[i]={{m.signed_half(source+8+i*2)*0.125f,m.signed_half(source+16+i*2)*0.125f,z}};
  }
  std::array<float,3> va{},vb{};for(unsigned i=0;i<3;++i){va[i]=expanded.vertices[1][i]-expanded.vertices[0][i];vb[i]=expanded.vertices[2][i]-expanded.vertices[0][i];}
  const auto cross=track_cross(va,vb);const auto squared=track_dot(cross,cross);
  if(!(squared>0)||!std::isfinite(squared))hp2::not_implemented("0x001510F8","unsupported degenerate/nonfinite VU track-polygon normalization");
  const float reciprocal=1.f/std::sqrt(squared);for(unsigned i=0;i<3;++i)expanded.plane[i]=cross[i]*reciprocal;
  expanded.plane[3]=(expanded.vertices[0][0]*expanded.plane[0]+expanded.vertices[0][1]*expanded.plane[1])+expanded.vertices[0][2]*expanded.plane[2];
  m.byte(polygon+11,m.byte(source+2));m.byte(polygon+8,0);m.byte(polygon+10,m.byte(source+3));m.word(polygon+0x10,0);
  for(unsigned i=0;i<4;++i)m.scalar(polygon+0x14+i*4,expanded.bounds[i]);
  for(unsigned i=0;i<4;++i)for(unsigned axis=0;axis<3;++axis)m.scalar(polygon+0x30+i*16+axis*4,expanded.vertices[i][axis]);
  m.byte(polygon+9,expanded.count);for(unsigned i=0;i<4;++i)m.scalar(polygon+0x70+i*4,expanded.plane[i]);
  // Original bGetSlotNumber002510A0, no expanded pools: InitTree disables overflow.
  m.half(source+6,static_cast<std::uint16_t>((polygon-pool-0x30)/m.word(pool+0x28)));m.word(polygon+12,source);
  const auto head=address_+0x24,previous=m.word(head+4);m.word(previous,polygon);m.word(head+4,polygon);m.word(polygon+4,previous);m.word(polygon,head);
 }else polygon=pool+0x30+static_cast<std::uint32_t>(number)*m.word(pool+0x28);
 m.byte(polygon+8,static_cast<std::uint8_t>(m.byte(polygon+8)+1));return polygon;
}
// Debug00152A58, return_closest_square=0; E-0099-W26.
std::uint32_t TopologyTree::GetSquare(float x,float y){
 auto& m=m_;auto node=m.word(address_);float left=-16384,bottom=-16384;
 for(unsigned level=10;level;--level){const float half=static_cast<float>(1u<<level)*32*0.5f;unsigned quadrant=0;
  if(left+half<=x){quadrant|=1;left+=half;}if(bottom+half<=y){quadrant|=2;bottom+=half;}
  const auto child=m.half(node+quadrant*2);if(!child)return 0;
  if(level==1)return m.word(address_+8)+child*12;node=m.word(address_)+child*8;
 }return 0;
}
// Debug00152E48/00152B88; E-0099-W26. Original mutable source deduplication and order.
std::vector<std::uint32_t> TopologyTree::BuildTrackPolygons(const std::array<float,2>& low,const std::array<float,2>& high,int maximum){
 auto& m=m_;std::vector<std::uint32_t> sources;
 if(-32768.f<low[0]&&high[0]<m.scalar(0x00324f78)&&-32768.f<low[1]&&high[1]<m.scalar(0x00324f78)){
  for(float y=hp2::recovered::TopologyTree::GetSquareCorner(low[1]);y<=high[1];y+=32)
   for(float x=hp2::recovered::TopologyTree::GetSquareCorner(low[0]);x<=high[0];x+=32){const auto square=GetSquare(x,y);if(!square)continue;
    const auto begin=m.half(square+8);const int end=begin+m.signed_half(square+10);
    for(int i=begin;i<end;++i){const auto source=m.word(address_+16)+m.half(m.word(address_+24)+static_cast<unsigned>(i)*2)*32;
     if(m.byte(source))continue;const auto bits=m.byte(source+1);
     const float minx=m.signed_half(source+8+(bits&3)*2)*0.125f,miny=m.signed_half(source+16+((bits>>2)&3)*2)*0.125f;
     const float maxx=m.signed_half(source+8+((bits>>4)&3)*2)*0.125f,maxy=m.signed_half(source+16+(bits>>6)*2)*0.125f;
     if(minx<=high[0]&&miny<=high[1]&&low[0]<=maxx&&low[1]<=maxy&&static_cast<int>(sources.size())<maximum){sources.push_back(source);m.byte(source,1);}
    }
   }
 }
 std::vector<std::uint32_t> out;for(auto source:sources){m.byte(source,0);out.push_back(AssignTrackPolygon(source));}return out;
}
// Debug001527E8; E-0099-W26. Original close frees all cached polygons.
void TopologyTree::CloseTree(){
 const auto pool=m_.word(0x00327394);if(!pool)return;const auto head=address_+0x24;
 while(m_.word(head)!=head){const auto node=m_.word(head),next=m_.word(node),previous=m_.word(node+4);m_.word(previous,next);m_.word(next+4,previous);
  const auto source=m_.word(node+12);if(source)m_.half(source+6,0xffff);SlotPool(m_,pool).Free(node);}
 bDeleteSlotPool(m_,pool);m_.word(0x00327394,0);
}
// Debug00151878; E-0099-W26.
void TopologyCoordinate::Sub_00151878(){
 for(unsigned offset=0x2a0;offset<=0x2ac;offset+=4)m_.word(address_+offset,0);
 m_.word(address_+0x2b4,0);m_.word(address_+0x2b8,0);m_.word(address_+0x2bc,0);ClearInterestBBox();
}
// Debug00151920 + bInitializeBoundingBox00252790; E-0099-W26.
void TopologyCoordinate::ClearInterestBBox(){
 auto& m=m_;const auto a=address_;for(unsigned off:{0u,0x10u}){
  m.word(a+off+4,m.word(0x00329054));m.word(a+off,m.word(0x00329054));
  m.word(a+off+12,m.word(0x00329058));m.word(a+off+8,m.word(0x00329058));}
 TopologyTree(m,0x002f0d10).RemoveTrackPolygons(a+0x20,m.signed_word(a+0x2a0));m.word(a+0x2b0,0);
 for(unsigned off=0x2a0;off<=0x2ac;off+=4)m.word(a+off,0);
}
// Debug00151980; E-0099-W26. Retains cached references until a bbox miss.
void TopologyCoordinate::SetInterestBBox(const std::array<float,2>& p,float radius,const std::array<float,2>* velocity){
 auto& m=m_;const auto a=address_;for(unsigned j=0;j<2;++j){m.scalar(a+j*4,p[j]-radius);m.scalar(a+8+j*4,p[j]+radius);}
 if(m.scalar(a+0x10)<=m.scalar(a)&&m.scalar(a+8)<=m.scalar(a+0x18)&&m.scalar(a+0x14)<=m.scalar(a+4)&&m.scalar(a+12)<=m.scalar(a+0x1c))return;
 TopologyTree tree(m,0x002f0d10);tree.RemoveTrackPolygons(a+0x20,m.signed_word(a+0x2a0));
 for(unsigned off=0x2a0;off<=0x2ac;off+=4)m.word(a+off,0);m.word(a+0x2b0,0);
 float scale=m.scalar(0x00324f64);const float padded=radius+0.5f;
 for(int attempt=0;attempt<(velocity?5:1);++attempt){
  for(unsigned j=0;j<2;++j){m.scalar(a+0x10+j*4,p[j]-padded);m.scalar(a+0x18+j*4,p[j]+padded);
   if(velocity){const auto future=p[j]+(*velocity)[j]*scale;m.scalar(a+0x10+j*4,std::min(m.scalar(a+0x10+j*4),future-padded));m.scalar(a+0x18+j*4,std::max(m.scalar(a+0x18+j*4),future+padded));}}
  const auto x=m.scalar(a+0x10)-m.scalar(a+0x18),y=m.scalar(a+0x14)-m.scalar(a+0x1c);
  if(!velocity||std::sqrt(x*x+y*y)<m.scalar(0x00324f60))break;scale*=0.75f;if(attempt==3)scale=0;
 }
 const auto polygons=tree.BuildTrackPolygons({{m.scalar(a+0x10),m.scalar(a+0x14)}},{{m.scalar(a+0x18),m.scalar(a+0x1c)}},160);
 unsigned at=0;for(auto polygon:polygons)if(!(m.byte(polygon+10)&(2|8)))m.word(a+0x20+at++*4,polygon);m.word(a+0x2a4,at);
 const auto horizontal=at;for(auto polygon:polygons)if(m.byte(polygon+10)&2)m.word(a+0x20+at++*4,polygon);m.word(a+0x2a8,at-horizontal);
 const auto vertical_end=at;if(at!=polygons.size())for(auto polygon:polygons)if(m.byte(polygon+10)&8)m.word(a+0x20+at++*4,polygon);
 m.word(a+0x2a0,static_cast<std::uint32_t>(polygons.size()));m.word(a+0x2ac,at-vertical_end);
}
// Debug00151CC0; E-0099-W26. Exact persistent polygon preference and table promotion.
float TopologyCoordinate::GetElevation(const std::array<float,3>& p,int* terrain,std::array<float,4>* normal,bool* valid){
 auto& m=m_;const auto a=address_;if(valid)*valid=true;
 const auto inside=[&](std::uint32_t poly){
  const auto edge=[&](unsigned x,unsigned y){return (m.scalar(poly+0x30+y*16+4)-m.scalar(poly+0x30+x*16+4))*(p[0]-m.scalar(poly+0x30+x*16))-(m.scalar(poly+0x30+y*16)-m.scalar(poly+0x30+x*16))*(p[1]-m.scalar(poly+0x30+x*16+4));};
  float value=std::max(-1.f,edge(0,1));if(value<=0)value=std::max(value,edge(1,2));if(value<=0)value=std::max(value,edge(3,0));if(value<=0&&m.byte(poly+9)==4)value=std::max(value,edge(2,3));return value;
 };
 const auto height=[&](std::uint32_t poly){return (m.scalar(poly+0x7c)-(m.scalar(poly+0x70)*p[0]+m.scalar(poly+0x74)*p[1]))/m.scalar(poly+0x78);};
 const auto copy_result=[&](std::uint32_t poly){if(terrain)*terrain=m.signed_byte(poly+11);if(normal)*normal=m.vector(poly+0x70);};
 const auto previous=m.word(a+0x2b0);if(previous&&inside(previous)<0){copy_result(previous);return height(previous);}
 if(!(m.scalar(a)<=p[0]&&p[0]<=m.scalar(a+8)&&m.scalar(a+4)<=p[1]&&p[1]<=m.scalar(a+12)))SetInterestBBox({{p[0],p[1]}},8);
 int selected=-1;float chosen=p[2],limit=p[2]+1;
 for(unsigned pass=0;pass<2;++pass){float best=m.scalar(0x0032738c);selected=-1;
  for(int i=0;i<m.signed_word(a+0x2a4);++i){const auto poly=m.word(a+0x20+static_cast<unsigned>(i)*4);
   if(m.scalar(poly+0x14)>p[0]||p[0]>m.scalar(poly+0x1c)||m.scalar(poly+0x18)>p[1]||p[1]>m.scalar(poly+0x20))continue;
   const auto side=inside(poly);if(side>0)continue;const auto z=height(poly);
   if(best<z&&z<limit){best=z;chosen=z;selected=i;if(!(m.byte(poly+10)&1)&&side<0){m.word(a+0x2b0,poly);break;}}
  }
  if(selected>=0)break;limit=m.scalar(0x00327390);
 }
 if(selected<0){if(valid)*valid=false;if(terrain)*terrain=1;if(normal){(*normal)[0]=0;(*normal)[2]=-1;(*normal)[1]=0;}return p[2];}
 const auto poly=m.word(a+0x20+static_cast<unsigned>(selected)*4);
 if(selected>4){for(int i=selected;i>0;--i)m.word(a+0x20+static_cast<unsigned>(i)*4,m.word(a+0x20+static_cast<unsigned>(i-1)*4));m.word(a+0x20,poly);}
 copy_result(poly);return chosen;
}
}
namespace port::track {
// Port-owned allocation adapter for the reviewed Retail/Debug00152860 raw schema.
TopologyResources load_retail_topology_resources(::port::driving::Memory& m,const std::vector<std::uint8_t>& bytes){
 constexpr std::uint32_t tree=0x002f0d10;
 if(m.word(tree+0x24)!=tree+0x24||m.word(tree+0x28)!=tree+0x24||!m.word(0x00327394))throw std::logic_error("TopologyTree constructor/InitTree must run before loading raw resources");
 for(unsigned off=0;off<32;off+=8)if(m.word(tree+off))throw std::logic_error("Original topology resources already loaded");
 Topology validation;validation.load(bytes); // Existing complete tree/index bounds validation.
 const auto chunks=hp2::assets::parse_chunks_top(bytes);TopologyResources out;out.owner=&m;
 for(unsigned i=0;i<4;++i){const auto* chunk=hp2::assets::find_first(chunks,0x34130+i);if(!chunk)throw std::runtime_error("Required raw topology chunk missing");
  const auto start=i<2?(chunk->header_offset+0x17)&~std::size_t(15):chunk->payload_offset();const auto size=chunk->payload_offset()+chunk->size-start;
  const auto raw=m.allocate(static_cast<std::uint32_t>(size));out.allocations.push_back(raw);std::memcpy(m.view(raw,size).data,bytes.data()+start,size);
  const unsigned stride=i==0?8:i==1?12:i==2?32:2;m.word(tree+i*8,raw);m.word(tree+i*8+4,static_cast<std::uint32_t>(size/stride));
  if(i==2)for(unsigned p=0;p<size;p+=32)m.half(raw+p+6,0xffff);
 }
 out.loaded=true;return out;
}
void unload_topology_resources(::port::driving::Memory& m,TopologyResources& resources){
 if(resources.owner!=&m||!resources.loaded)throw std::invalid_argument("Topology resource owner/lifetime mismatch");
 if(m.word(0x00327394))throw std::logic_error("CloseTree must release expanded polygon references before raw topology disposal");
 for(unsigned i=0;i<32;i+=4)m.word(0x002f0d10+i,0);
 for(auto p:resources.allocations)m.release(p);resources={};
}
}

namespace hp2::debug {
// Debug001518C0; E-0099-W26. Unlike ClearInterestBBox, the destructor does not reset fields.
void TopologyCoordinate::Sub_001518C0(int in_chrg){TopologyTree(m_,0x002f0d10).RemoveTrackPolygons(address_+0x20,m_.signed_word(address_+0x2a0));if(in_chrg&1)m_.release(address_);}
}

#include "game_core/port/track/position_markers.hpp"
namespace hp2::debug {
// Debug00151018; TASK-0099 E-0099-W34. Exact authored list order/hash matching.
int GetNumTrackPositionMarkers(const ::port::driving::Memory& m,std::uint32_t hash){int count=0;for(auto p=m.word(0x327378);p!=0x327378;p=m.word(p))if(m.word(p+8)==hash)++count;return count;}
// Debug00151058; E-0099-W34. Index counts matching hashes only; misses return0.
std::uint32_t GetTrackPositionMarker(const ::port::driving::Memory& m,std::uint32_t hash,int index){int count=0;for(auto p=m.word(0x327378);p!=0x327378;p=m.word(p))if(m.word(p+8)==hash){if(count==index)return p;++count;}return 0;}
// Debug00153258, E-0099-W34. Marker34146 branch only, no whole track loader claim.
int LoaderTrackGeometry(::port::driving::Memory& m,std::uint32_t chunk){
 if(m.word(chunk)!=0x34146)hp2::not_implemented("0x00153258","non-marker LoaderTrackGeometry branch requires its actual resource owner");
 for(unsigned i=0;i<m.word(chunk+4)/24;++i){const auto node=chunk+8+24*i,tail=m.word(0x32737c);m.word(tail,node);m.word(node+4,tail);m.word(0x32737c,node);m.word(node,0x327378);}return 1;
}
// Debug00153368, E-0099-W34. Unlink records; backing bytes belong to bundle.
int UnloaderTrackGeometry(::port::driving::Memory& m,std::uint32_t chunk){
 if(m.word(chunk)!=0x34146)hp2::not_implemented("0x00153368","non-marker UnloaderTrackGeometry branch requires its actual resource owner");
 for(unsigned i=0;i<m.word(chunk+4)/24;++i){const auto node=chunk+8+24*i,next=m.word(node),prev=m.word(node+4);m.word(prev,next);m.word(next+4,prev);}return 1;
}
}

namespace hp2::debug {
// Debug001521A8, E-0099-W39. Uses this coordinate's persistent actual cache.
bool TopologyCoordinate::HasTopology(const std::array<float,2>& p){bool valid=false;GetElevation({{p[0],p[1],m_.scalar(0x324f68)}},nullptr,nullptr,&valid);return valid;}
}

namespace hp2::debug {
// Debug00151428, E-0099-W50. Read actual expanded vertices/plane directly;
// no independent polygon cache or re-normalized Retail adapter is used.
bool TrackPolygon::PointInShaft(const std::array<float,4>& point,float* elevation){const int count=m_.signed_byte(address_+9);if(count!=3&&count!=4)hp2::not_implemented("0x00151428","original shaft test with unsupported vertex count/uninitialized stack rays");std::array<std::array<float,3>,4> rays{};for(int i=0;i<count;++i)for(unsigned axis=0;axis<3;++axis)rays[i][axis]=m_.scalar(address_+0x30+std::uint32_t(i)*16+axis*4)-point[axis];const std::array<float,3> normal{{m_.scalar(address_+0x70),m_.scalar(address_+0x74),m_.scalar(address_+0x78)}};for(int i=0;i<count;++i)if(track_dot(track_cross(rays[i],rays[(i+1)%count]),normal)<0)return false;if(elevation)*elevation=-track_dot(rays[0],normal);return true;}
// Debug001515E0, E-0099-W50. Strict opposite-plane-side test; endpoint touches
// are rejected. Ordered interpolation retains all four original vector lanes.
bool TrackPolygon::SegmentIntersects(const std::array<float,4>& first,const std::array<float,4>& second,std::array<float,4>* intersection){std::array<float,3> a{},b{},normal{};for(unsigned axis=0;axis<3;++axis){const auto origin=m_.scalar(address_+0x30+axis*4);a[axis]=first[axis]-origin;b[axis]=second[axis]-origin;normal[axis]=m_.scalar(address_+0x70+axis*4);}const float da=track_dot(a,normal),db=track_dot(b,normal);if(!(da*db<0))return false;const float ratio=std::abs(da)/(std::abs(da)+std::abs(db));std::array<float,4> point{};for(unsigned lane=0;lane<4;++lane)point[lane]=(second[lane]-first[lane])*ratio+first[lane];if(!PointInShaft(point))return false;if(intersection)*intersection=point;return true;}
// Debug001516D8, E-0099-W50. Original bBoundingBoxOverlapping00252C10 uses
// STRICT XYZ overlap, unlike older bounded camera prose claiming inclusive.
bool TrackPolygon::TestSegmentToPoly(float& nearest,const std::array<float,4>& target,const std::array<float,4>& position,const std::array<float,4>& low,const std::array<float,4>& high,bool two_sided){auto minimum=m_.vector(address_+0x30),maximum=minimum;for(int i=1;i<m_.signed_byte(address_+9);++i)for(unsigned axis=0;axis<3;++axis){const auto value=m_.scalar(address_+0x30+std::uint32_t(i)*16+axis*4);minimum[axis]=std::min(minimum[axis],value);maximum[axis]=std::max(maximum[axis],value);}for(unsigned axis=0;axis<3;++axis)if(!(low[axis]<maximum[axis]&&minimum[axis]<high[axis]))return false;std::array<float,4> intersection{};if(!SegmentIntersects(target,position,&intersection))return false;std::array<float,3> difference{},normal{};for(unsigned axis=0;axis<3;++axis){difference[axis]=intersection[axis]-target[axis];normal[axis]=m_.scalar(address_+0x70+axis*4);}if(!two_sided&&0<=track_dot(difference,normal))return false;const auto distance=std::sqrt(track_dot(difference,difference));if(distance<nearest){nearest=distance;return true;}return false;}
// Debug00152500, E-0099-W50. Uses this coordinate's existing shared polygon
// table and references; never builds a substitute topology or independent cache.
bool TopologyCoordinate::ConstrainToTopology(eConstraintType type,bool two_sided,const std::array<float,4>& target,std::array<float,4>& position,float margin,float wall_up){auto low=target,high=target;for(unsigned axis=0;axis<3;++axis){low[axis]=std::min(low[axis],position[axis]-margin);high[axis]=std::max(high[axis],position[axis]+margin);}float nearest=m_.scalar(0x324f74);std::uint32_t chosen=0;for(int i=0;i<m_.signed_word(address_+0x2a0);++i){const auto polygon=m_.word(address_+0x20+std::uint32_t(i)*4);if(TrackPolygon(m_,polygon).TestSegmentToPoly(nearest,target,position,low,high,two_sided))chosen=polygon;}
 std::array<float,4> boom{};for(unsigned lane=0;lane<4;++lane)boom[lane]=position[lane]-target[lane];const float distance=std::sqrt((boom[0]*boom[0]+boom[1]*boom[1])+boom[2]*boom[2]);if(!(nearest<distance))return false;
 const auto normalize=[&](float length){const auto normalized=hp2::recovered::bNormalize({{boom[0],boom[1],boom[2]}},length);for(unsigned axis=0;axis<3;++axis)boom[axis]=normalized[axis];};
 if(type==eCONSTRAIN_BY_ROTATION){const auto flags=m_.byte(chosen+0xa);if(flags&2){const float length=nearest-margin;normalize(length);boom[2]+=wall_up*(1-length/distance);for(unsigned lane=0;lane<4;++lane)position[lane]=target[lane]+boom[lane];return true;}const float plane=(m_.scalar(chosen+0x7c)-(m_.scalar(chosen+0x70)*position[0]+m_.scalar(chosen+0x74)*position[1]))/m_.scalar(chosen+0x78);position[2]=(flags&8)?plane-margin:plane+margin;
 }else if(type==eCONSTRAIN_BY_SCALE){normalize(nearest-margin);for(unsigned lane=0;lane<4;++lane)position[lane]=target[lane]+boom[lane];}return true;
}
}
