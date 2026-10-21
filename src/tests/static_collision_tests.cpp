#include "game_core/port/driving/native_backend.hpp"
#include "game_core/port/driving/static_collisions.hpp"
#include "game_core/indep/inc/CollisionBody.hpp"
#include "game_core/indep/src/rigidbody.hpp"
#include "platform/ps2_asset_root.hpp"
#include "port/assets/catalog.hpp"
#include "port/vehicle/catalog.hpp"
#include "game_core/indep/inc/Polyhedra.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
namespace {void check(bool value,const char* message){if(!value)throw std::runtime_error(message);}}
namespace {
void alias_volumes(port::assets::Source& source,port::driving::Memory& memory,const std::vector<std::uint8_t>& global){
    port::vehicle::Catalog catalogue(source);catalogue.load();unsigned aliases=0,declared=0,unavailable=0;
    for(const auto& car:catalogue.cars()){
        if(car.code==car.prefix)continue;
        ++declared;
        if(!car.available){++unavailable;std::cout<<"COLLISION_ALIAS_UNAVAILABLE "<<car.code<<' '<<car.availability_error<<'\n';continue;}
        const auto config=port::driving::read_config(global,car.code);
        check(port::driving::base_model_name(config)==car.prefix,"Native collision prefix differs from original car catalogue BaseModelName");
        std::array<float,3> cog;for(unsigned i=0;i<3;++i){std::uint32_t bits=0;for(unsigned j=0;j<4;++j)bits|=std::uint32_t(config.raw[0x110+i*4+j])<<(j*8);std::memcpy(&cog[i],&bits,4);}
        const auto geometry=source.read(car.resource_path);auto volumes=port::track::load_collision_volumes(geometry);
        const auto expected=hp2::recovered::FindPipelinePolyhedron(volumes,hp2::frontend::bStringHash((car.prefix+"_CV").c_str()),&cog);
        check(expected!=nullptr,"Aliased car is missing its original base-model collision volume");
        port::driving::StaticCollisions actual;actual.configure(memory,config,cog,geometry);
        check(actual.shape.vertices.size()==expected->shape.vertices.size()&&actual.shape.edges.size()==expected->shape.edges.size()&&actual.shape.faces.size()==expected->shape.faces.size(),"Aliased native car silently selected fallback collision shape");
        for(unsigned i=0;i<actual.shape.vertices.size();++i)check(actual.shape.vertices[i].position==expected->shape.vertices[i].position,"Aliased car collision shape or CG correction differs");
        ++aliases;
    }
    check(declared==21&&aliases+unavailable==declared,"Original HP alias catalogue coverage changed");
    check(aliases>=2,"At least two original shared-model aliases are required for collision-volume regression");
    auto invalid=port::driving::read_config(global,"911TURBO");std::fill_n(invalid.raw.begin()+0x20,32,'X');
    bool rejected=false;try{port::driving::base_model_name(invalid);}catch(const std::exception&){rejected=true;}
    check(rejected,"Unterminated BaseModelName was accepted");
    std::cout<<"NATIVE_COLLISION_MODEL_ALIASES_OK declared="<<declared<<" tested="<<aliases<<" unavailable="<<unavailable<<'\n';
}
}
int main(){try{
    const auto root=hp2::platform::resolve_zzdata_root(".");if(!root)return 77;
    std::filesystem::path elf;try{elf=port::driving::debug_image_path(".");}catch(const std::exception&){return 77;}
    port::assets::DirectorySource source(*root);const auto global=source.read("GLOBAL/GLOBALB.BUN");const auto config=port::driving::read_config(global,"911TURBO");
    port::driving::NativeBackend backend(elf);backend.advance(config,{},1.0f/60);
    auto m=backend.memory();const auto mover=backend.physics_car_address(),body=m.word(mover+0x18),car_state=m.word(mover+0x50),cfg=m.word(car_state),car=car_state-0x40;
    alias_volumes(source,m,global);
    port::driving::StaticCollisions collisions;collisions.configure(m,config,{{m.scalar(cfg+0x110),m.scalar(cfg+0x114),m.scalar(cfg+0x118)}},source.read("CARS/911TURBO/GEOMETRY.BIN"));
    float left=100;for(const auto& v:collisions.shape.vertices)left=std::min(left,v.position[0]);
    port::track::Topology topology;topology.nodes.resize(10);for(unsigned i=0;i<9;++i)topology.nodes[i].fill(static_cast<std::uint16_t>(i+1));topology.nodes[9].fill(1);topology.squares.resize(2);topology.squares[1][5]=1;topology.indices={0};
    port::track::Polygon wall;wall.count=4;wall.flags=2;wall.terrain=1;wall.bounds={{0,-50,0,50}};wall.vertices={{{{0,-50,-5}},{{0,50,-5}},{{0,50,5}},{{0,-50,5}}}};wall.plane={{1,0,0,0}};topology.polygons.push_back(wall);
    const std::array<float,16> identity{{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}};
    m.matrix(body+0x20,identity);m.vector(body+0x10,{{-left-0.1f,0,0,1}});m.vector(body+0x60,{{-10*m.scalar(body+0x118),0,0,0}});m.vector(body+0x70,{{0,0,0,0}});hp2::recovered::RigidBody(m,body).ComputeAuxiliaries();
    auto motion=hp2::recovered::DynamicCollisionBody::BeginMove(m,body,0.03f);motion.previous.position[0]+=0.3f;
    const auto contacts=collisions.contacts(m,body,topology,motion);check(!contacts.empty(),"Loaded car shape did not contact original wall wafer");
    const auto before=m.vector(body+0x60);unsigned moved=0;
    // Explicit fixture callback: isolate original contact backup/impulse ordering; no invented motion/bounce.
    const auto move=[&](float remaining){check(remaining>=0&&remaining<=0.03f,"Invalid original remaining movement interval");++moved;};
    const auto recompute=[&]{hp2::recovered::RigidBody(m,body).ComputeAuxiliaries();};
    collisions.resolve(m,body,car,contacts.front(),motion,move,recompute);
    check(collisions.resolved_contacts==1&&collisions.applied_impulses==1&&moved==1,"Original static contact impulse was not dispatched");
    check(m.scalar(body+0x60)>before[0]&&collisions.last_impulse[0]>0,"Wall impulse did not oppose incoming car momentum");
    auto environment=std::make_shared<port::driving::TrackEnvironment>();environment->label="Explicit controlled wall test";environment->direction={{1,0}};environment->start={{0,0,0}};
    environment->topology.nodes=topology.nodes;environment->topology.squares.resize(2);environment->topology.squares[1][5]=2;environment->topology.indices={0,1};
    port::track::Polygon road;road.count=4;road.terrain=1;road.bounds={{-500,-100,500,100}};road.vertices={{{{-500,-100,0}},{{500,-100,0}},{{500,100,0}},{{-500,100,0}}}};road.plane={{0,0,1,0}};
    wall.bounds={{20,-100,20,100}};for(auto& v:wall.vertices)v[0]=20;wall.plane={{-1,0,0,-20}};
    // Reverse winding for the original inward-facing wall normal.
    std::swap(wall.vertices[1],wall.vertices[3]);environment->topology.polygons={road,wall};
    port::driving::NativeBackend driving(elf);driving.collision_geometry(source.read("CARS/911TURBO/GEOMETRY.BIN"));driving.track(environment);
    bool impact=false;float prior_speed=0;unsigned impact_step=0,audio_requests=0;
    for(unsigned step=0;step<600;++step){const auto snapshot=driving.advance(config,{1,0,0},1.0f/60);const auto& runtime=driving.static_collisions();
        audio_requests+=snapshot.collision_sounds.size();
        if(runtime.applied_impulses&&!impact){check(runtime.last_impulse[0]<0,"Controlled wall did not oppose forward car momentum");check(snapshot.velocity->at(0)<prior_speed,"Controlled impact did not change original native velocity");impact=true;impact_step=step;}
        if(impact&&step>impact_step+20)break;
        prior_speed=snapshot.velocity->at(0);}
    check(audio_requests>0,"Native wall collision did not publish delayed original impact sound");
    const auto requests_before=audio_requests;
    for(unsigned step=0;step<20;++step)audio_requests+=driving.advance(config,{1,0,0},1.0f/60).collision_sounds.size();
    check(audio_requests==requests_before,"Persistent wall contact restarted an allocated impact sample");
    check(impact,"Native driving did not dispatch original wall collision response");
    std::cout<<"FULL_NATIVE_WALL_IMPACT_OK impulses="<<driving.static_collisions().applied_impulses<<'\n';
    auto corner=std::make_shared<port::driving::TrackEnvironment>(*environment);corner->direction={{0.70710678f,0.70710678f}};
    auto second_wall=wall;second_wall.bounds={{-100,20,100,20}};second_wall.vertices={{{{-100,20,-5}},{{100,20,-5}},{{100,20,5}},{{-100,20,5}}}};second_wall.plane={{0,-1,0,-20}};
    corner->topology.polygons.push_back(second_wall);corner->topology.indices.push_back(2);corner->topology.squares[1][5]=3;
    port::driving::NativeBackend corner_driving(elf);corner_driving.collision_geometry(source.read("CARS/911TURBO/GEOMETRY.BIN"));corner_driving.track(corner);bool both=false;
    for(unsigned step=0;step<650;++step){corner_driving.advance(config,{1,0,0},1.0f/60);const auto& hits=corner_driving.static_collisions().impulses_by_polygon;
        if(hits.count(1)&&hits.count(2)){both=true;break;}}
    check(both,"Original contact recheck did not resolve both corner walls");std::cout<<"FULL_NATIVE_CORNER_IMPACT_OK\n";
    // TASK0087: water now follows original static-body constants; CAR terrain
    // still requires unrecovered ownership and preserves this rollback boundary.
    auto unsupported=std::make_shared<port::driving::TrackEnvironment>(*environment);unsupported->topology.polygons[1].terrain=32;
    port::driving::NativeBackend failed(elf),control(elf);for(auto* b:{&failed,&control})b->collision_geometry(source.read("CARS/911TURBO/GEOMETRY.BIN"));failed.track(unsupported);control.track(environment);
    bool rejected=false;
    for(unsigned step=0;step<600&&!rejected;++step){const auto before_position=failed.physics_car_address()?failed.memory().vector(failed.memory().word(failed.physics_car_address()+0x18)+0x10):std::array<float,4>{};
        const auto contacts_before=failed.static_collisions().resolved_contacts,witnesses_before=failed.static_collisions().witnesses.size(),audio_before=failed.static_collisions().audio_contacts.size();
        const auto seed_before=failed.physics_car_address()?failed.memory().word(0x327eec):0;
        try{failed.advance(config,{1,0,0},1.0f/60);}catch(const std::exception&){
            rejected=true;const auto failed_body=failed.memory().word(failed.physics_car_address()+0x18);
            check(failed.memory().vector(failed_body+0x10)==before_position&&failed.static_collisions().resolved_contacts==contacts_before&&failed.static_collisions().witnesses.size()==witnesses_before,"Collision failure changed committed native memory or witness diagnostics");
            check(failed.static_collisions().audio_contacts.size()==audio_before&&failed.memory().word(0x327eec)==seed_before,"Rejected collision changed audio lifetime or random state");
            unsupported->topology.polygons[1].terrain=1;const auto retry=failed.advance(config,{1,0,0},1.0f/60),expected=control.advance(config,{1,0,0},1.0f/60);
            check(retry.body==expected.body&&retry.velocity==expected.velocity&&failed.static_collisions().applied_impulses==control.static_collisions().applied_impulses,"Rejected collision candidate changed subsequent original state");break;
        }control.advance(config,{1,0,0},1.0f/60);
    }
    check(rejected,"Unsupported material contact did not fail explicitly");std::cout<<"NATIVE_COLLISION_ROLLBACK_OK\n";

    std::cout<<"STATIC_TRACK_COLLISION_OK: original CV/VClip/backup/impulse chain, isolated move callback, not original runtime verification\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
