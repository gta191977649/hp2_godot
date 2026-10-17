#include "game_core/port/track/topology.hpp"
#include "game_core/indep/src/track.hpp"
#include "game_core/indep/inc/CollisionBody.hpp"
#include "game_core/indep/inc/Polyhedra.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
namespace {
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
void unit(){
    using hp2::recovered::TopologyTree;using hp2::recovered::TrackPolygon;
    check(TopologyTree::GetSquareCorner(-0.01f)==-32,"Negative fixed point corner");
    check(TopologyTree::GetSquareCorner(32)==32,"Positive fixed point corner");
    std::array<std::uint8_t,32> packed{};packed[1]=0x90;packed[2]=6;packed[3]=0x10;
    const auto put=[&](unsigned at,int value){packed[at]=value&255;packed[at+1]=(value>>8)&255;};
    // Counterclockwise XY with an upward plane and 1-unit elevation.
    put(8,0);put(10,80);put(12,80);put(14,0);
    put(16,0);put(18,0);put(20,80);put(22,80);put(4,1);
    auto p=TrackPolygon::Sub_001514A8(packed.data());
    check(p.count==4&&p.terrain==6&&p.plane[2]==1&&p.plane[3]==1,"Packed polygon conversion");
    port::track::VoronoiPolyhedron wafer;hp2::recovered::WaferPolyhedron::Init(wafer,p,{{0,0,0}});
    check(wafer.vertices.size()==4&&wafer.edges.size()==4&&wafer.faces.size()==2,"Quad wafer adjacency");
    check(wafer.edges[0].direction[0]==8192&&wafer.edges[0].vertices[1]==1,"Original edge quantization");
    check(wafer.faces[0].planes[0].normal[1]>0&&wafer.faces[0].plane.distance==1,"Wafer face Voronoi plane");
    auto triangle=p;triangle.vertices[3]=triangle.vertices[2];hp2::recovered::WaferPolyhedron::Init(wafer,triangle,{{0,0,0}});
    check(wafer.vertices.size()==3&&wafer.edges[2].vertices[1]==2,"Triangle wafer adjacency");
    float distance=0;check(TrackPolygon::PointInShaft(p,{{5,5,3}},&distance)&&distance==2,"Shaft distance");
    check(!TrackPolygon::PointInShaft(p,{{15,5,3}},nullptr),"Shaft outside");
    port::track::Topology t;t.nodes.resize(10);for(unsigned i=0;i<9;++i)t.nodes[i]={{static_cast<std::uint16_t>(i+1),static_cast<std::uint16_t>(i+1),static_cast<std::uint16_t>(i+1),static_cast<std::uint16_t>(i+1)}};
    t.nodes[9]={{1,1,1,1}};t.squares.resize(2);t.squares[1][5]=1;t.polygons.push_back(p);t.indices.push_back(0);
    auto hit=t.elevation({{5,5,2}});check(hit.found&&hit.height==1&&hit.terrain==6,"Real query publication");
    check(!t.elevation({{15,5,2}}).found,"No plane fallback outside polygon");
    auto lower=p;for(auto& v:lower.vertices)v[2]=-2;lower.plane[3]=-2;t.polygons.push_back(lower);t.indices.push_back(1);t.squares[1][5]=2;
    hit=t.elevation({{5,5,-1.5f}});check(hit.found&&hit.height==-2,"Layer below query threshold");
    std::array<float,3> intersection{};
    check(TrackPolygon::SegmentIntersects(p,{{5,5,3}},{{5,5,-1}},&intersection)&&intersection[2]==1,"Segment crossing plane");
    check(!TrackPolygon::SegmentIntersects(p,{{5,5,1}},{{5,5,-1}},nullptr),"Original strict plane crossing");
    port::track::CollisionCylinder cylinder;cylinder.center={{5,5,1.1f}};cylinder.base_height=0.2f;cylinder.top_height=0.2f;cylinder.radius=1;
    check(hp2::recovered::DynamicCollisionBody::TrackPolygonCheck(cylinder,p),"Cylinder near plane");
    cylinder.center[2]=3;check(!hp2::recovered::DynamicCollisionBody::TrackPolygonCheck(cylinder,p),"Cylinder outside plane");
    cylinder.center={{5,5,1.1f}};check(!t.collision_candidates(cylinder).empty(),"Collision candidate publication");
    // Loader reads records relative to the original header alignment, not host vector alignment.
    port::track::Topology decoded;std::vector<std::uint8_t> chunk(40);std::copy(packed.begin(),packed.end(),chunk.begin()+8);
    check(TopologyTree::Loader(decoded,0x34132,chunk,0,32)&&decoded.polygons.size()==1,"Canonical chunk loader");
    check(TopologyTree::Unloader(decoded,0x34132)&&decoded.polygons.empty(),"Canonical chunk unloader");
    bool bad=false;try{TopologyTree::Loader(decoded,0x34132,chunk,0,31);}catch(const std::exception&){bad=true;}check(bad,"Invalid polygon stride rejected");
    bad=false;try{TopologyTree::Loader(decoded,0x34130,chunk,35,8);}catch(const std::exception&){bad=true;}check(bad,"Truncated chunk rejected");
    const auto old=t.polygons.size();bool rejected=false;try{t.load({1,2,3});}catch(const std::exception&){rejected=true;}
    check(rejected&&t.polygons.size()==old,"Failed replacement keeps prior topology");t.clear();check(!t.ready(),"Unload clears owned data");
}
}
int main(int argc,char** argv){try{unit();for(int a=1;a<argc;++a){std::ifstream f(argv[a],std::ios::binary);std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(f)),{});port::track::Topology t;t.load(data);std::size_t found=0;for(std::size_t i=0;i<t.polygons.size();++i){const auto& p=t.polygons[i];if(p.flags&(2|8))continue;std::array<float,3> q{};for(unsigned j=0;j<p.count;++j)for(unsigned k=0;k<3;++k)q[k]+=p.vertices[j][k]/p.count;q[2]+=0.5f;if(t.elevation(q).found)++found;}std::cout<<argv[a]<<" polygons="<<t.polygons.size()<<" ground_centroid_hits="<<found<<'\n';check(found>0,"Real track has road contact");}std::cout<<"PASS track topology\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

