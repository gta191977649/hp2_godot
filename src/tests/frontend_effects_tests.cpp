#include "game_core/src/acidfxe.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/port/track/particles.hpp"
#include "game_core/port/driving/original_image.hpp"
#include "game_core/port/not_implemented.hpp"
#include "platform/ps2_asset_root.hpp"
#include "port/assets/catalog.hpp"
#include "game_core/indep/src/frontendscreens/selectcar.hpp"
#include "game_core/indep/inc/AcidFX.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
void near(float a,float b,float tolerance,const char* message){require(std::abs(a-b)<=tolerance,message);}
port::track::ParticleDefinition fixture(){
    port::track::ParticleDefinition d;d.name_hash=1;d.key_positions={{0,1,2,3}};
    for(int i=0;i<4;++i){const float t=float(i)/3;
        d.colour_matrix[i*4]=2*t*t*t-3*t*t+5*t+7;
        d.colour_matrix[i*4+1]=10*t*t;
        d.colour_matrix[i*4+2]=20-5*t;
        d.colour_matrix[i*4+3]=60;
        d.size[i]=3+t;d.relative_angle[i]=float(i)*.25f;
        d.extra_basis[i*4+2]=123;d.extra_basis[i*4+3]=456;
    }
    return d;
}
}
int main(){
    using namespace port::driving;
    SolverMatrix general{{1,2,3,4,0,2,1,2,0,0,3,1,2,0,0,4}},inverse{},product{};
    hp2::debug::fInvertMatrix(inverse,general);hp2::recovered::eMulMatrix(product,general,inverse);
    for(int i=0;i<16;++i)near(product[i],i%5==0?1.f:0.f,.00001f,"non-affine general inverse failed identity");
    SolverMatrix singular{},unchanged;unchanged.fill(123);
    require(!hp2::guard([&]{hp2::debug::fInvertMatrix(unchanged,singular);}).empty() && unchanged[0]==123,
        "singular general inverse published unsupported host result");
    Memory memory;memory.map(0x324c80,16);memory.scalar(0x324c84,0x1.555556p-2f);memory.scalar(0x324c88,0x1.6c16c2p+7f);
    auto d=fixture();port::track::rebuild_frontend_bases(d,memory);
    for(int sample=0;sample<=30;++sample){const float t=float(sample)/30;
        const auto colour=hp2::recovered::hermite_parameter(d.colour_basis,t);
        near(colour[0],2*t*t*t-3*t*t+5*t+7,.0002f,"frontend colour cubic interpolation differs");
        near(colour[1],10*t*t,.0002f,"frontend colour quadratic interpolation differs");
        near(colour[2],20-5*t,.0002f,"frontend colour linear interpolation differs");
        near(colour[3],60,.0002f,"frontend colour constant alpha differs");
        const auto extra=hp2::recovered::hermite_parameter(d.extra_basis,t);
        near(extra[0],3+t,.0002f,"frontend size basis differs");
        near(extra[1],t*.75f*memory.scalar(0x324c88),.002f,"frontend relative-angle basis scale differs");
    }
    for(int i=0;i<4;++i)require(d.extra_basis[i*4+2]==123 && d.extra_basis[i*4+3]==456,
        "unresolved original extra stack lanes were invented");
    SolverMatrix alias=d.colour_matrix,expected{};
    hp2::debug::hermite_basis(expected,alias,0,1,2,3);
    hp2::debug::hermite_basis(alias,alias,0,1,2,3);require(alias==expected,"basis output alias changed source");
    port::track::ParticleResources resources;resources.definitions.push_back(fixture());
    port::track::ParticleSimulation racing(resources,42),frontend(resources,42);
    racing.step(memory,1.f/60,{});frontend.step(memory,1.f/60,{},true);
    require(racing.resources().definitions[0].colour_basis==SolverMatrix{} && frontend.resources().definitions[0].colour_basis==d.colour_basis,
        "frontend basis rebuild changed default world step or was skipped");
    auto bad=fixture();bad.key_positions.fill(1);resources.definitions.push_back(bad);
    port::track::ParticleSimulation rollback(resources,42);
    require(!hp2::guard([&]{rollback.step(memory,1.f/60,{},true);}).empty()
        && rollback.resources().definitions[0].colour_basis==SolverMatrix{} && rollback.global_seed()==42,
        "failed second basis rebuild committed partial frontend state");
    port::track::ParticleSimulation gpu(resources,42);gpu.set_gpu_emission(true);
    require(!hp2::guard([&]{gpu.step(memory,1.f/60,{},true);}).empty(),"unbound frontend GPU dynamic bases silently used stale data");
    {
        port::track::ParticleResources library;library.definitions.push_back(fixture());
        port::track::SharedParticleState ledger{42};port::track::ParticleSimulation registry(library,ledger);
        port::track::ParticleEmitter emitter;emitter.name_hash=emitter.data_hash=1;emitter.state=1;
        const auto outside=registry.add_auto_emitter(emitter,ledger);
        std::vector<std::size_t> handles;
        for(int i=0;i<601;++i)handles.push_back(registry.add_auto_emitter(emitter,ledger));
        require(registry.auto_emitter_order().size()==602 && registry.resources().emitters.size()==602,
            "initial original600-emitter pool was treated as a hard limit");
        registry.remove_auto_emitter(handles[100]);
        const auto reused=registry.add_auto_emitter(emitter,ledger);
        require(reused==handles[100] && registry.auto_emitter_order().front()==reused,
            "detached inactive storage was not reused with original head insertion");
        for(auto handle:handles)registry.remove_auto_emitter(handle);
        const auto seed=ledger.seed;registry.kill_all_active_effects(ledger);
        require(registry.resources().emitters.size()==1 && registry.auto_emitter_order()==std::vector<std::size_t>{outside}
            && ledger.seed==seed,"registry cleanup changed unrelated registration/RNG or retained dead tail");
        port::track::ParticleEmitter absent;absent.data_hash=99;absent.state=1;
        const auto missing=registry.add_auto_emitter(absent,ledger);
        require(registry.resources().emitters[missing].state==2 && registry.auto_emitter_order().back()==missing,
            "missing definition did not follow original State2 tail registration");
        registry.remove_auto_emitter(missing);registry.remove_auto_emitter(outside);registry.kill_all_active_effects(ledger);
        require(registry.resources().emitters.empty(),"final registry storage did not return to baseline");
    }
    const auto path=debug_image_path(".");const auto root=hp2::platform::resolve_zzdata_root(".");
    if(path.empty() || !std::filesystem::exists(path) || !root){std::puts("PASS frontend effect math/transaction; SKIP original resources");return 0;}
    auto original=read_debug_image(path);port::assets::DirectorySource source(*root);
    auto authored=port::track::decode_particles(source.read("GLOBAL/GLOBALB.BUN"));
    const auto found=std::find_if(authored.definitions.begin(),authored.definitions.end(),[](const auto& value){return value.name=="XE_SelectCarNebula";});
    require(found!=authored.definitions.end(),"original frontend nebula definition absent");
    auto actual=*found;port::track::rebuild_frontend_bases(actual,original);
    for(int key=0;key<4;++key){const float t=actual.key_positions[key]*original.scalar(0x324c84);
        const auto colour=hp2::recovered::hermite_parameter(actual.colour_basis,t);
        const auto extra=hp2::recovered::hermite_parameter(actual.extra_basis,t);
        for(int lane=0;lane<4;++lane)near(colour[lane],actual.colour_matrix[key*4+lane],.02f,"original nebula colour control point changed");
        near(extra[0],actual.size[key],.02f,"original nebula size control point changed");
        near(extra[1],actual.relative_angle[key]*original.scalar(0x324c88),.04f,"original nebula angle control point changed");
    }
    // Original constructor allocates20 zeroed State1 emitters referencing one
    // definition. Do not auto-register the library's unrelated template emitters.
    const auto nebula_definition=std::size_t(found-authored.definitions.begin());
    const auto prior_basis=found->colour_basis;
    authored.emitters.clear();
    for(int i=0;i<20;++i){port::track::ParticleEmitter emitter;emitter.name="XE_SelectCarNebula";
        emitter.name_hash=emitter.data_hash=found->name_hash;emitter.state=1;authored.emitters.push_back(emitter);}
    port::track::ParticleSimulation nebula(std::move(authored),original.word(0x326d78));
    require(nebula.auto_emitter_order().size()==20 && nebula.auto_emitter_order().front()==19
        && nebula.auto_emitter_order().back()==0,"original nebula construction reversed auto-emitter order lost");
    std::array<hp2::debug::CarTypeInfo,64> cars{};cars[0].NumAvailableSkinNumbers=1;
    hp2::debug::FrontendPlayerInfo player;
    player.SetCarAvailability(hp2::debug::eCAR_LOCKED_FULLY_VISIBLE,0,-1,false,cars,0);
    port::selection::ListServices lists{cars,[](int,bool){return false;},[](int){return std::string("fixture");},false};
    hp2::debug::VisibleCarSet list(player,9,false,lists);hp2::debug::CarSelector selector{list};
    selector.NumNebulaEmitters=20;for(int i=0;i<20;++i)selector.NebulaEmitters[i]=std::size_t(i);
    const auto colours=port::selection::load_nebula_colours(original);
    selector.UpdateNebula(original,nebula,colours);selector.UpdateNebula(original,nebula,colours);
    near(selector.NebulaColourParam,original.scalar(0x3255b0)*2,0,"constructor and first Tick must both update nebula colour");
    const auto& definition=nebula.resources().definitions[nebula_definition];
    require(definition.colour_basis==prior_basis && definition.colour_matrix!=colours.unlocked,
        "nebula Tick rebuilt basis before the original after-display boundary");
    for(const auto& emitter:nebula.resources().emitters){const auto& matrix=emitter.local_world;
        near(matrix[12]*matrix[12]+matrix[13]*matrix[13],225,.1f,"nebula original15-unit ring differs");
        require(matrix[14]==-1.25f && matrix[15]==1,"nebula original Z/W translation differs");}
    const auto failure=hp2::guard([&]{nebula.step(original,original.scalar(0x324a48),{},true);});
    require(failure.empty(),failure.c_str());
    require(nebula.particle_count()==20,"first original nebula step did not emit20 particles");
    // Check the birth before the separate original gravity integration changes
    // its direction. The original nebula has gravity=-.05, not a frozen pose.
    port::track::ParticleEmitter birth;birth.state=1;birth.random_seed=42;birth.local_world=hp2::recovered::bIdentity();
    std::vector<port::track::Particle> newborn;std::uint32_t birth_seed=42;
    hp2::recovered::AcidEffectSystem::TriggerEmitter(original,birth,actual,original.scalar(0x324a48),{},birth_seed,newborn,1000,true);
    require(newborn.size()==1 && newborn[0].speed==0 && newborn[0].direction==std::array<std::int8_t,3>{},
        "original zero-speed nebula birth packed direction differs");
    selector.AllCarsUnlocked=1;selector.UpdateNebula(original,nebula,colours);
    require(selector.NebulaColourParam<original.scalar(0x3255b0)*2,"unlock override did not reverse nebula colour transition");
    port::track::ParticleResources capacity;capacity.definitions.push_back(actual);
    capacity.definitions[0].controls[4]=100000;capacity.definitions[0].controls[5]=0;
    port::track::ParticleEmitter emitter;emitter.name_hash=emitter.data_hash=actual.name_hash;
    emitter.state=1;emitter.local_world=hp2::recovered::bIdentity();capacity.emitters.push_back(emitter);
    port::track::SharedParticleState shared{42};
    port::track::ParticleSimulation first(capacity,shared),second(capacity,shared);
    bool missing_ledger=false;
    try{first.step(original,original.scalar(0x324a48),{},true);}catch(const std::logic_error&){missing_ledger=true;}
    require(missing_ledger && shared.particles==0,"shared system silently stepped with a private pool budget");
    require(!hp2::guard([&]{first.trigger_dynamic(original,0,1,{},emitter.local_world,{});}).empty(),
        "unbound shared dynamic trigger silently bypassed global accounting");
    first.step_shared(shared,original,original.scalar(0x324a48),{},true);
    require(first.particle_count()==1000 && shared.particles==1000 && shared.groups==1,"shared original particle pool failed first fill");
    const auto before_seed=shared.seed;
    second.step_shared(shared,original,original.scalar(0x324a48),{},true);
    require(second.particle_count()==0 && shared.particles==1000 && shared.groups==1 && shared.seed==before_seed,
        "second effect system received another1000 slots or changed global RNG instead of emitter RNG");
    first.remove_auto_emitter(0);first.kill_all_active_effects(shared);
    require(shared.particles==0 && shared.groups==0 && first.auto_emitter_order().empty(),"selector effect teardown retained global pool ownership");
    second.step_shared(shared,original,original.scalar(0x324a48),{},true);
    require(second.particle_count()>0 && shared.particles==second.particle_count(),"freed shared particle slots were not reusable");
    auto invalid_shared=shared;invalid_shared.particles=0;
    const auto count=second.particle_count();const auto old_seed=second.global_seed();bool rejected=false;
    try{second.step_shared(invalid_shared,original,original.scalar(0x324a48),{},true);}catch(const std::logic_error&){rejected=true;}
    require(rejected && second.particle_count()==count && second.global_seed()==old_seed,"invalid shared ownership partially advanced system");
    second.kill_all_active_effects(shared);require(shared.particles==0 && shared.groups==0,"final shared pool release differs");
    capacity.definitions[0].controls[4]=1;capacity.emitters.assign(401,emitter);
    port::track::ParticleSimulation group_limit(capacity,shared),group_second(capacity,shared);
    group_limit.step_shared(shared,original,original.scalar(0x324a48),{},true);
    require(shared.groups==400 && shared.particles==400,"original shared400-group pool limit differs");
    group_second.step_shared(shared,original,original.scalar(0x324a48),{},true);
    require(group_second.groups().empty() && shared.groups==400,"second system exceeded shared group capacity");
    group_limit.kill_all_active_effects(shared);group_second.kill_all_active_effects(shared);
    std::puts("PASS native frontend effect bases, finite inverse, transactions and original nebula control points; not original runtime parity");
}
