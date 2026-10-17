#include "game_core/port/track/debug_geometry.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
namespace {
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
void near(float a,float b,const char* message){check(std::abs(a-b)<0.0001f,message);}
template<class F>void rejects(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"Malformed debug geometry was accepted");}
port::track::Polygon quad(float x,float base,int terrain,std::uint8_t flags=0){
    port::track::Polygon p;p.count=4;p.flags=flags;p.terrain=terrain;p.bounds={{x,0,x+8,8}};
    p.vertices={{{{x,0,base}},{{x+8,0,base+2}},{{x+8,8,base+2}},{{x,8,base}}}};
    const float inverse=1.0f/std::sqrt(17.0f);p.plane={{-inverse,0,4*inverse,(4*base-x)*inverse}};return p;
}
port::track::Topology queryable(std::vector<port::track::Polygon> polygons){
    port::track::Topology t;t.polygons=std::move(polygons);t.nodes.resize(10);
    for(unsigned i=0;i<9;++i)t.nodes[i].fill(static_cast<std::uint16_t>(i+1));t.nodes[9].fill(1);t.squares.resize(2);
    t.squares[1][5]=static_cast<std::uint16_t>(t.polygons.size());for(unsigned i=0;i<t.polygons.size();++i)t.indices.push_back(static_cast<std::uint16_t>(i));return t;
}
std::vector<port::driving::SurfaceMaterial> catalog(){return {{1,"Fixture road"},{2,"Fixture dirt"},{6,"Fixture grass"},{0,"Fixture default"}};}
void verify_planes(const port::track::Topology& topology,const port::track::DebugGeometry& geometry){
    for(const auto& triangle:geometry.triangles){const auto& p=topology.polygons.at(triangle.polygon);for(const auto& v:triangle.vertices)
        near((p.plane[3]-(p.plane[0]*v[0]+p.plane[1]*v[1]))/p.plane[2],v[2],"Overlay vertex differs from original contact plane");}
}
void unit(){
    auto lower=quad(0,0,1,1),upper=quad(0,10,6,1),neighbor=quad(8,2,2);
    // Raw v4 is intentionally off-plane: the original query still uses the stored plane.
    lower.vertices[3][2]=20;const auto original=lower.vertices;
    auto wall=quad(24,0,2,2),ceiling=quad(32,0,2,8),combined=quad(40,0,2,10),vertical=quad(48,0,1);vertical.plane={{1,0,0,48}};
    auto triangle=quad(64,3,0);triangle.count=3;triangle.vertices[3]=triangle.vertices[2];triangle.bounds={{64,0,72,8}};
    auto unknown=quad(80,0,99);auto topology=queryable({lower,upper,neighbor,wall,ceiling,combined,vertical,triangle,unknown});
    const auto geometry=port::track::build_debug_geometry(topology,catalog());
    check(geometry.triangles.size()==9&&geometry.edges.size()==19&&geometry.skipped_polygons==4,"Eligibility/triangle/quad output counts");
    check(topology.polygons[0].vertices==original,"Diagnostic projection mutated physical topology");
    check(geometry.legend.size()==5&&geometry.legend.front().id==0&&geometry.legend.back().id==99,"Stable encountered material legend");
    const auto& missing=geometry.legend.back();check(!missing.known&&missing.name=="Unknown"&&missing.color==port::track::unknown_debug_color,"Unknown material is explicit magenta");
    for(std::size_t i=0;i<port::driving::surface_terrain_ids.size();++i)check(port::track::debug_material_color(port::driving::surface_terrain_ids[i],true)==port::driving::surface_colors[i],"Test drive material colors changed");
    for(const auto& row:geometry.legend)check(row.polygon_count==1,"Legend count includes skipped polygons or loses road layers");
    verify_planes(topology,geometry);near(geometry.triangles[1].vertices[2][2],0,"Noncoplanar fourth vertex was not projected");
    // Preserve original polygon perimeter only: the fan's 0->2 diagonal must never become an edge.
    for(const auto& edge:geometry.edges)if(edge.polygon==0){const auto& a=edge.vertices[0];const auto& b=edge.vertices[1];check(a[0]==b[0]||a[1]==b[1],"Internal triangulation diagonal leaked into outline");}
    unsigned boundary_edges=0;for(const auto& edge:geometry.edges)if(edge.vertices[0][0]==8&&edge.vertices[1][0]==8&&(edge.polygon==0||edge.polygon==2))++boundary_edges;
    check(boundary_edges==2,"Adjacent material perimeters were merged or lost");
    const auto low=topology.elevation({{2,2,0}}),high=topology.elevation({{2,2,12}});
    check(low.found&&low.polygon==0&&low.terrain==1&&high.found&&high.polygon==1&&high.terrain==6,"Original multi-layer contact selection changed");
    near(low.height,0.5f,"Slope contact elevation");near(high.height,10.5f,"Upper layer contact elevation");
    check(geometry.triangles[0].terrain==low.terrain&&geometry.triangles[2].terrain==high.terrain,"Overlay material differs from original selected polygon");
    const auto adjacent_contact=topology.elevation({{10,2,3}});check(adjacent_contact.found&&adjacent_contact.terrain==2&&geometry.triangles[4].terrain==adjacent_contact.terrain,"Adjacent material boundary query mismatch");
    const auto triangle_contact=topology.elevation({{70,2,4}});check(triangle_contact.found&&triangle_contact.polygon==7&&triangle_contact.terrain==0,"Triangle overlay lost original eligible contact");
    const auto all_unknown=port::track::build_debug_geometry(queryable({quad(0,0,1)}),{});check(!all_unknown.legend[0].known&&all_unknown.triangles[0].color==port::track::unknown_debug_color,"Missing catalogue guessed a material name/color");
    auto duplicate=catalog();duplicate.push_back(duplicate.front());rejects([&]{port::track::build_debug_geometry(topology,duplicate);});
    auto bad=lower;bad.count=2;rejects([&]{port::track::build_debug_geometry(queryable({bad}),catalog());});
    bad=lower;bad.plane[2]=std::numeric_limits<float>::quiet_NaN();rejects([&]{port::track::build_debug_geometry(queryable({bad}),catalog());});
    bad=lower;bad.vertices[0][0]=std::numeric_limits<float>::infinity();rejects([&]{port::track::build_debug_geometry(queryable({bad}),catalog());});
    check(port::track::build_debug_geometry({},catalog()).triangles.empty(),"Empty topology invented geometry");
}
}
int main(int argc,char** argv){try{unit();for(int i=1;i<argc;++i){std::ifstream file(argv[i],std::ios::binary);if(!file)throw std::runtime_error("Debug geometry bundle is missing");std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)),{});port::track::Topology topology;topology.load(bytes);const auto geometry=port::track::build_debug_geometry(topology,catalog());verify_planes(topology,geometry);check(!geometry.triangles.empty(),"Original track produced no contact debug faces");std::cout<<argv[i]<<" triangles="<<geometry.triangles.size()<<" perimeter_edges="<<geometry.edges.size()<<'\n';}std::cout<<"TRACK_DEBUG_GEOMETRY_OK: contact plane projection, eligibility, layers, material mapping and perimeter outlines\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
