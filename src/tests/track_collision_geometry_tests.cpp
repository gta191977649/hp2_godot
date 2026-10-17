#include "game_core/indep/inc/Polyhedra.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void validate(const port::track::VoronoiPolyhedron& p){
    check(!p.vertices.empty()&&!p.edges.empty()&&!p.faces.empty(),"Nonempty collision polyhedron");
    for(const auto& v:p.vertices){check(v.edges.size()==v.planes.size(),"Vertex Voronoi adjacency");for(auto edge:v.edges)check(edge<p.edges.size(),"Vertex edge index");}
    for(const auto& f:p.faces){check(f.edges.size()==f.planes.size(),"Face Voronoi adjacency");for(const auto& plane:f.planes)for(float v:plane.normal)check(std::isfinite(v),"Finite face plane");}
    for(const auto& e:p.edges)for(const auto& plane:e.planes){check(std::isfinite(plane.distance),"Finite edge plane distance");for(float v:plane.normal)check(std::isfinite(v),"Finite edge plane");}
}
void unit(){
    port::track::Polygon polygon;polygon.count=4;polygon.vertices={{{{0,0,1}},{{10,0,1}},{{10,10,1}},{{0,10,1}}}};polygon.plane={{0,0,1,1}};
    port::track::VoronoiPolyhedron wafer;hp2::recovered::WaferPolyhedron::Init(wafer,polygon,{{0,0,0}});validate(wafer);
    check(wafer.vertices.size()==4&&wafer.edges.size()==4&&wafer.faces.size()==2,"Quad wafer counts");
    check(wafer.edges[0].direction[0]==8192&&wafer.faces[0].planes[0].normal[1]==1,"Original direction quantization");
    polygon.vertices[3]=polygon.vertices[2];hp2::recovered::WaferPolyhedron::Init(wafer,polygon,{{0,0,0}});validate(wafer);check(wafer.vertices.size()==3,"Triangle wafer adjacency");
    hp2::recovered::RectPolyhedron::Sub_00212EB0(wafer,2,-2,1,-1,0.5f,-0.5f);validate(wafer);
    check(wafer.vertices.size()==8&&wafer.edges.size()==12&&wafer.faces.size()==6,"Original rectangular fallback counts");
    check(wafer.vertices[0].position==std::array<float,3>{{2,1,0.5f}}&&wafer.vertices[7].position==std::array<float,3>{{-2,-1,-0.5f}},"Original rectangular vertex order");
    port::track::VoronoiPolyhedron cube;hp2::recovered::RectPolyhedron::Sub_00212EB0(cube,1,-1,1,-1,1,-1);
    port::track::VoronoiPose pose_a,pose_b;pose_b.position={{3,0,0}};port::track::VoronoiWitness witness;
    auto status=hp2::recovered::VCWitness::VClip(witness,cube,pose_a,cube,pose_b);
    check(status==1&&std::abs(witness.distance-1)<0.001f,"Separated convex boxes");
    pose_b.position={{1.5f,0,0}};status=hp2::recovered::VCWitness::VClip(witness,cube,pose_a,cube,pose_b);check(status==2,"Penetrating convex boxes");
    pose_b.position={{4,2,0}};status=hp2::recovered::VCWitness::VClip(witness,cube,pose_a,cube,pose_b);check(status==1&&witness.distance>=2,"Persistent witness separates after penetration");
    polygon.vertices[3]={{0,10,1}};hp2::recovered::WaferPolyhedron::Init(wafer,polygon,{{0,0,0}});
    witness={};pose_a.position={{5,5,3}};pose_b={};status=hp2::recovered::VCWitness::VClip(witness,cube,pose_a,wafer,pose_b);
    check(status==1&&std::abs(witness.distance-1)<0.001f,"Convex box separated from wafer");
    pose_a.position[2]=1.5f;status=hp2::recovered::VCWitness::VClip(witness,cube,pose_a,wafer,pose_b);check(status==2,"Convex box penetrates wafer");
    hp2::recovered::RectPolyhedron::Sub_00212EB0(wafer,2,-2,1,-1,0.5f,-0.5f);
    port::track::CollisionVolume volume;volume.shape=wafer;volume.name_hash=123;std::vector<port::track::CollisionVolume> volumes{volume};std::array<float,3> cog{{0.5f,0,0}};
    auto* selected=hp2::recovered::FindPipelinePolyhedron(volumes,123,&cog);check(selected&&selected->shape.vertices[0].position[0]==1.5f,"Center of gravity shift");
    hp2::recovered::FindPipelinePolyhedron(volumes,123,&cog);check(selected->shape.vertices[0].position[0]==1.5f,"Center of gravity shift once");
    check(hp2::recovered::FindPipelinePolyhedron(volumes,321,nullptr)==nullptr,"Missing pipeline volume is explicit");
    port::track::VoronoiImage invalid;invalid.vertices.push_back(1);bool rejected=false;try{hp2::recovered::VCPolyhedron::MemoryImageLoad(wafer,invalid);}catch(const std::exception&){rejected=true;}check(rejected,"Truncated collision record rejected");
}
}
int main(int argc,char** argv){try{unit();for(int i=1;i<argc;++i){std::ifstream f(argv[i],std::ios::binary);std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(f)),{});auto volumes=port::track::load_collision_volumes(bytes);for(const auto& v:volumes){validate(v.shape);
        port::track::Polygon ground;ground.count=4;ground.vertices={{{{-100,-100,0}},{{100,-100,0}},{{100,100,0}},{{-100,100,0}}}};ground.plane={{0,0,1,0}};
        port::track::VoronoiPolyhedron wafer;hp2::recovered::WaferPolyhedron::Init(wafer,ground,{{0,0,0}});
        port::track::VoronoiPose car_pose,track_pose;car_pose.position[2]=10;port::track::VoronoiWitness witness;
        check(hp2::recovered::VCWitness::VClip(witness,v.shape,car_pose,wafer,track_pose)==1,"Real collision shape above track wafer");
        float minimum=10000,maximum=-10000;for(const auto& vertex:v.shape.vertices){minimum=std::min(minimum,vertex.position[2]);maximum=std::max(maximum,vertex.position[2]);}
        car_pose.position[2]=-(minimum+maximum)*0.5f;
        check(hp2::recovered::VCWitness::VClip(witness,v.shape,car_pose,wafer,track_pose)==2,"Real collision shape intersects track wafer");
        std::cout<<v.name<<" vertices="<<v.shape.vertices.size()<<" edges="<<v.shape.edges.size()<<" faces="<<v.shape.faces.size()<<'\n';}}std::cout<<"PASS collision geometry\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
