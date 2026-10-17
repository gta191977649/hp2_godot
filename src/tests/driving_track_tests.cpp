#include "game_core/port/driving/native_backend.hpp"
#include "game_core/indep/src/quickspline.hpp"
#include "platform/ps2_asset_root.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <set>
namespace {
void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class F>void rejected(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"Unsupported track input was accepted");}
void units(){
    port::track::Spline line;line.points={{{0,0}},{{10,0}},{{20,0}}};
    auto p=hp2::recovered::QuickSpline::GetPoint(line,0.5f,0);check(p[0]==5&&p[1]==0,"Original spline midpoint");
    auto d=hp2::recovered::QuickSpline::GetPoint(line,-2,1);check(d[0]==10&&d[1]==0,"Original extrapolated derivative");
    check(std::abs(hp2::recovered::QuickSpline::FindClosestParam(line,{{7,3}},0,0.05f)-0.7f)<0.01f,"Original closest-param iteration");
    for(int i=0;i<33;++i)if(i!=4&&i!=5&&i!=20&&i!=32)check(port::driving::debug_terrain_from_retail(i)==i,"Terrain semantic map");
    for(int i:{-1,4,5,20,32,33})rejected([&]{port::driving::debug_terrain_from_retail(i);});
    rejected([]{port::driving::load_track_environment(port::track::Scene{});});
}
void real_material_queries(std::shared_ptr<const port::driving::TrackEnvironment> environment){
    port::driving::TrackContact contact;contact.environment=environment;
    port::driving::Memory memory;const auto point=memory.allocate(16),material=memory.allocate(4),plane=memory.allocate(16),found=memory.allocate(4);
    std::set<int> types;
    for(const auto& polygon:environment->topology.polygons){
        if(polygon.flags&10||polygon.terrain==4||polygon.terrain==5||polygon.terrain==20||polygon.terrain==32)continue;
        std::array<float,3> center{};for(unsigned i=0;i<polygon.count;++i)for(unsigned j=0;j<3;++j)center[j]+=polygon.vertices[i][j]/polygon.count;
        center[2]+=0.1f;const auto expected=environment->topology.elevation(center);if(!expected.found)continue;
        if(expected.terrain==4||expected.terrain==5||expected.terrain==20||expected.terrain==32)continue;
        memory.vector(point,{{center[0],center[1],center[2],1}});
        const auto height=contact.GetElevation(memory,0,point,material,plane,found);
        check(height==expected.height&&memory.word(found)==1&&memory.word(material)==std::uint32_t(expected.terrain)&&memory.vector(plane)==expected.plane,"Real polygon contact normal/material publication");
        types.insert(expected.terrain);if(types.size()>=3)break;
    }
    check(types.size()>=2,"Real track samples did not include multiple road materials");
    std::cout<<"REAL_MATERIAL_SAMPLES "<<types.size()<<'\n';
}
}
int main(){try{
    units();const auto root=hp2::platform::resolve_zzdata_root(".");if(!root){std::cout<<"Original resources unavailable\n";return 77;}
    port::assets::DirectorySource source(*root);const auto entries=port::track::load_catalog(source);
    check(!entries.empty(),"Original track catalogue is empty");std::shared_ptr<const port::driving::TrackEnvironment> first;
    unsigned loaded=0,blocked=0;
    for(const auto& entry:entries){port::track::Scene scene;scene.entry=entry;scene.bundle=port::track::decompress_bundle(source.read(entry.resources.at(1)));
        try{auto env=port::driving::load_track_environment(scene);check(env->topology.ready(),"Track topology not loaded");
            check(std::abs(env->direction[0]*env->direction[0]+env->direction[1]*env->direction[1]-1)<0.0001f,"Starting direction not normalized");
            if(!first)first=env;
            ++loaded;std::cout<<"START "<<entry.number<<' '<<env->start[0]<<' '<<env->start[1]<<' '<<env->start[2]<<'\n';
        }catch(const std::exception& error){++blocked;std::cout<<"BLOCKED "<<entry.number<<' '<<error.what()<<'\n';}
    }
    check(first!=nullptr,"No original marker/path/topology start can be loaded");
    real_material_queries(first);
    const auto elf=port::driving::debug_image_path(".");const auto config=port::driving::read_config(source.read("GLOBAL/GLOBALB.BUN"),"911TURBO");
    auto backend=std::make_shared<port::driving::NativeBackend>(elf);backend->collision_geometry(source.read("CARS/911TURBO/GEOMETRY.BIN"));port::driving::Session session(backend);session.configure(config);session.track(first);
    check(session.track_label()==first->label,"Track identity missing");
    for(unsigned i=0;i<120;++i){check(session.single_step(),session.diagnostic().c_str());const auto& snapshot=session.snapshot();check(snapshot.track_label==first->label&&snapshot.track_material_catalog.size()==33&&!snapshot.surface,"Real track publication");}
    for(const auto& wheel:*session.snapshot().wheels)check(wheel.contact&&*wheel.contact,"Real wheel did not settle on topology");
    const auto track_position=*session.snapshot().body;check(std::abs(track_position[12]-first->start[0])<10&&std::abs(track_position[13]-first->start[1])<10,"Original start did not reach native pose");
    session.recording(true);
    for(unsigned i=0;i<15;++i){session.input({0.1f,0,0});check(session.single_step(),session.diagnostic().c_str());}
    const auto recorded=session.snapshot();session.replay();
    for(unsigned i=0;i<15;++i)check(session.single_step(),session.diagnostic().c_str());
    check(session.snapshot().body==recorded.body&&session.snapshot().velocity==recorded.velocity&&session.snapshot().track_label==first->label,"Real track replay identity or motion changed");
    session.track({});check(session.track_label().empty()&&session.snapshot().step==0&&session.tape().empty(),"Track clear lifecycle");check(session.single_step(),session.diagnostic().c_str());check(session.snapshot().surface.has_value(),"Synthetic scenario not restored");
    std::cout<<"DRIVING_TRACK_OK loaded="<<loaded<<" blocked="<<blocked<<"; port acceptance only, not original equivalence\n";
}catch(const std::exception& error){std::cerr<<"DRIVING_TRACK_FAILED: "<<error.what()<<'\n';return 1;}}
