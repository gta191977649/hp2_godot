#include "game_core/port/driving/native_backend.hpp"
#include "platform/ps2_asset_root.hpp"
#include "port/assets/catalog.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace port::driving;
namespace {
void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F>void rejects(F f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}check(caught,"Expected rejected test surface request");}
bool same_grip(const GripSnapshot& a,const GripSnapshot& b){
    return a.terrain==b.terrain&&a.slipping==b.slipping&&a.z_force==b.z_force&&a.current_slip==b.current_slip
        &&a.traction_usage==b.traction_usage&&a.traction_boost==b.traction_boost&&a.catchup_friction==b.catchup_friction
        &&a.road_noise==b.road_noise&&a.slip==b.slip&&a.ground_force==b.ground_force
        &&a.static_coefficient==b.static_coefficient&&a.dynamic_coefficient==b.dynamic_coefficient;
}
bool same(const Snapshot& a,const Snapshot& b){
    if(a.step!=b.step||a.dt!=b.dt||a.body!=b.body||a.velocity!=b.velocity||a.angular_velocity!=b.angular_velocity||a.surface!=b.surface)return false;
    if(!a.wheels||!b.wheels)return false;
    for(unsigned i=0;i<4;++i){const auto& x=(*a.wheels)[i];const auto& y=(*b.wheels)[i];
        if(x.tire!=y.tire||x.brake!=y.brake||x.blur!=y.blur||x.phase!=y.phase||x.angular_velocity!=y.angular_velocity
           ||x.contact!=y.contact||x.blur_alpha!=y.blur_alpha||!x.grip||!y.grip||!same_grip(*x.grip,*y.grip))return false;
    }
    return true;
}
void layout(){
    SurfaceSettings zones{SurfaceMode::Zones,1};
    check(surface_material_at(zones,-1000)==1&&surface_material_at(zones,0)==1,"Negative coordinate is first zone");
    for(unsigned i=0;i<surface_boundaries.size();++i){const auto x=surface_boundaries[i];
        check(surface_material_at(zones,std::nextafter(x,-std::numeric_limits<float>::infinity()))==surface_terrain_ids[i],"Boundary lower side");
        check(surface_material_at(zones,x)==surface_terrain_ids[i+1],"Boundary belongs to following zone");
    }
    check(surface_material_at(zones,10000)==8,"Last zone extends outward");
    Memory m;const auto point=m.allocate(16),material=m.allocate(4),normal=m.allocate(16),found=m.allocate(4);
    PlaneContact provider;provider.settings=zones;provider.height=2;
    for(float x:{24.f,26.f,49.f,51.f}){m.vector(point,{{x,1,3,1}});provider.GetElevation(m,0,point,material,normal,found);
        check(m.signed_word(material)==surface_material_at(zones,x)&&m.word(found)==1&&m.vector(normal)==std::array<float,4>{{0,0,1,2}},"Each wheel query independently resolves the shared layout");}
    for(int id:surface_terrain_ids){provider.settings={SurfaceMode::Uniform,id};m.vector(point,{{-100,0,0,1}});provider.GetElevation(m,0,point,material,normal,0);check(m.signed_word(material)==id,"Uniform test surface ID");}
    rejects([]{validate_surface({SurfaceMode::Uniform,5});});rejects([]{validate_surface({SurfaceMode(8),1});});
    rejects([&]{surface_material_at(zones,std::numeric_limits<float>::quiet_NaN());});
}
void inspect(const NativeBackend& backend,const Snapshot& s){
    check(s.surface&&s.surface_catalog&&s.wheels,"Production surface publication must be complete");
    const auto& m=backend.memory();const auto mover=backend.physics_car_address(),state=m.word(mover+0x50);
    for(unsigned i=0;i<4;++i){const auto a=m.word(mover+0x30+4*i);const auto& w=(*s.wheels)[i];
        check(w.grip&&w.contact,"Complete four-wheel diagnostics");const auto& g=*w.grip;
        check(g.terrain==m.signed_word(state+0x314+4*i)&&g.z_force==m.scalar(state+0x2d0+4*i)
           &&g.traction_usage==m.scalar(state+0x354+4*i)&&g.slip[0]==m.scalar(state+0x334+8*i),"Diagnostics agree with original UpdateCar publication");
        check(same_grip(g,observe_grip(m,a)),"Diagnostics observe same completed Move state");
        if(s.surface->mode==SurfaceMode::Uniform)check(g.terrain==s.surface->terrain,"Original Wheel receives selected terrain");
    }
}
float speed(const Snapshot& s){const auto& v=*s.velocity;return std::sqrt(v[0]*v[0]+v[1]*v[1]);}
void scenarios(const std::filesystem::path& elf,const Config& config){
    NativeBackend backend(elf);std::array<Snapshot,5> endpoints;
    for(unsigned material=0;material<5;++material){backend.reset();const SurfaceSettings surface{SurfaceMode::Uniform,surface_terrain_ids[material]};Snapshot s;
        for(unsigned i=0;i<360;++i){const Input input{i>=30&&i<150?0.8f:(i>=240?0.6f:0.f),i>=195&&i<240?0.6f:0.f,i>=240?0.2f:0.f};
            s=backend.advance(config,input,1.f/60,surface);inspect(backend,s);
            if(i==149||i==194||i==239||i==359)std::cout<<"surface "<<surface.terrain<<" phase "<<i<<" speed "<<speed(s)<<" force "<<(*s.wheels)[0].grip->ground_force[0]<<','<<(*s.wheels)[0].grip->ground_force[1]<<'\n';
        }
        endpoints[material]=s;
    }
    check(endpoints[0].body!=endpoints[4].body,"Original terrain inputs produce distinct trajectories without a guessed target difference");
    const auto& cat=*endpoints[0].surface_catalog;
    check(cat[0].id==1&&cat[0].name=="ROAD"&&cat[0].static_friction==1&&cat[0].dynamic_friction==1,"Debug ROAD catalogue");
    check(cat[3].rolling_friction==2&&cat[4].static_friction==0.95f,"Debug SAND/SNOW exact scalar anchors");
    backend.reset();bool split=false;std::array<bool,5> visited{};
    for(unsigned i=0;i<780;++i){const auto s=backend.advance(config,{1,0,0},1.f/60,{SurfaceMode::Zones,1});inspect(backend,s);
        int first=(*s.wheels)[0].grip->terrain;
        for(const auto& w:*s.wheels){if(w.grip->terrain!=first)split=true;for(unsigned j=0;j<5;++j)if(w.grip->terrain==surface_terrain_ids[j])visited[j]=true;}
    }
    check(split,"Driving across a boundary produces mixed front/rear wheel materials");
    for(bool present:visited)check(present,"Driving crosses every material zone");
}
void switching_replay_failure(const std::filesystem::path& elf,const Config& config){
    auto backend=std::make_shared<NativeBackend>(elf);Session session(backend);session.configure(config);session.recording(true);session.run();std::vector<Snapshot> tape;
    for(unsigned i=0;i<120;++i){const auto old=session.snapshot();session.surface({i>=90?SurfaceMode::Zones:SurfaceMode::Uniform,surface_terrain_ids[(i/30)%5]});session.input({0.7f,0,i>70?0.1f:0.f});
        if(old.step)check(same(old,session.snapshot()),"Queued surface choice cannot mutate the last committed publication");
        check(session.tick(1.f/30),"Native surface recording step");inspect(*backend,session.snapshot());
        check(session.snapshot().step==old.step+1,"Surface change does not restart the simulation");tape.push_back(session.snapshot());}
    session.recording(false);session.surface({SurfaceMode::Uniform,8});session.replay();session.run();
    for(const auto& expected:tape){check(session.tick(1.f/30),"Surface replay step");check(same(session.snapshot(),expected),"Replay preserves controls, elapsed, surface and every diagnostic");}
    check(!session.tick(1.f/30)&&session.paused(),"Replay terminates without extra frame");
    const auto before=session.snapshot();rejects([&]{session.surface({SurfaceMode::Uniform,32});});check(same(before,session.snapshot()),"Invalid material leaves publication intact");
    NativeBackend tested(elf),control(elf);Snapshot prefix;
    for(unsigned i=0;i<60;++i){prefix=tested.advance(config,{.7f,0,0},1.f/60);control.advance(config,{.7f,0,0},1.f/60);}
    const auto a=tested.physics_car_address(),time=tested.memory().word(0x327ce4),gear=tested.memory().word(tested.memory().word(a+0x44)+0x30);
    rejects([&]{tested.advance(config,{.7f,0,1.e30f},1.f/60,{SurfaceMode::Uniform,8});});
    check(tested.memory().word(0x327ce4)==time&&tested.memory().word(tested.memory().word(a+0x44)+0x30)==gear,"Rejected candidate preserves committed clock/gear");
    const SurfaceSettings dirt{SurfaceMode::Uniform,2};
    check(same(tested.advance(config,{.7f,0,0},1.f/60,dirt),control.advance(config,{.7f,0,0},1.f/60,dirt)),"Material candidate failure cannot perturb subsequent complete state");
    check(prefix.step==60,"Failed candidate did not alter held snapshot");
}
}
int main(){try{
    layout();const auto root=hp2::platform::resolve_zzdata_root(".");if(!root){std::cout<<"Original data unavailable\n";return 77;}
    std::filesystem::path elf;try{elf=debug_image_path(".");}catch(const std::exception& e){std::cout<<e.what()<<'\n';return 77;}
    port::assets::DirectorySource source(*root);const auto config=read_config(source.read("GLOBAL/GLOBALB.BUN"),"911TURBO");
    scenarios(elf,config);switching_replay_failure(elf,config);
    std::cout<<"DRIVING_SURFACES_OK: original algorithms/materials, synthetic layout, not PS2 equivalence\n";
}catch(const std::exception& e){std::cerr<<"DRIVING_SURFACES_FAILED: "<<e.what()<<'\n';return 1;}}
