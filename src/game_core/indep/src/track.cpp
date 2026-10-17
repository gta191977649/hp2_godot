
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
