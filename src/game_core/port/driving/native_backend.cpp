#include "native_backend.hpp"
#include "game_core/indep/inc/PhysicsCar.hpp"
#include "game_core/indep/inc/Car.hpp"
#include "game_core/indep/inc/CollisionBody.hpp"
#include "game_core/indep/src/carstate.hpp"
#include "game_core/indep/inc/RealDriver.hpp"
#include "game_core/indep/src/engine.hpp"
#include "game_core/indep/src/skids.hpp"
#include "game_core/indep/src/wheel.hpp"
#include "game_core/ReplayManager.hpp"
#include "game_core/indep/src/timer.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/port/driving/projections.hpp"
#include "game_core/port/driving/frame_lifecycle.hpp"
#include "game_core/port/driving/horn.hpp"
#include "game_core/port/not_implemented.hpp"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <limits>
namespace port::driving {
native::InitServices NativeBackend::init_services(){
    native::InitServices s;s.contact=contact();s.Field_00137938_stack_w=0;
    s.Sub_00250F38=[] {return static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());};
    s.Sub_00250F48=[](std::uint64_t start,std::uint64_t end){return static_cast<std::uint32_t>(end-start);};
    s.ResolveForces=[](Memory& m,std::uint32_t a){hp2::recovered::PhysicsCar(m,a).ResolveForces();};
    // The fixture does not install an external race/police predicate service.
    return s;
}
native::DrivingServices NativeBackend::driving_services(native::InitServices& init){
    native::DrivingServices s;s.Sub_00250F38=init.Sub_00250F38;s.Sub_00250F48=init.Sub_00250F48;s.Field_001389A4_stack_w=0;
    s.IntegrateEuler=[&init](Memory& m,std::uint32_t a,float dt){native::integrate(m,a,dt,init);};
    s.eOrthonormalizeMatrix=[](Memory& m,std::uint32_t a){hp2::recovered::eOrthonormalizeMatrix(m,a);};
    s.UpdateCar=[](Memory& m,std::uint32_t a,float dt){hp2::recovered::PhysicsCar(m,a).UpdateCar(dt);};
    s.IsJoystickTypeWheel=[](std::int32_t){return false;}; // Explicit tool controller kind.
    s.MaybeMakeSkids=[this](Memory& m,std::uint32_t wheel,std::uint32_t car,float dt){
        SkidServices service{&skid_arena_,contact(),{}};
        hp2::recovered::Wheel(m,wheel).MaybeMakeSkids(dt,car,service);
    };
    return s;
}
void NativeBackend::reset(){initialized_=false;horn_input_=false;step_=0;skid_arena_={};collisions_.clear();collision_sound_selector_.reset();tire_audio_mixer_.reset();splash_audio_mixer_.reset();if(loaded_)state_=image_;}
void NativeBackend::track(std::shared_ptr<const TrackEnvironment> value){
    if(value&&!value->topology.ready())throw std::invalid_argument("Track environment has no loaded topology");
    track_contact_.environment=std::move(value);reset();
}
float NativeBackend::minimum_frame_elapsed()const{
    if(!loaded_)return 0;
    // Host pacing uses the bound read by Debug PrepareRealTimestep001A0C50.
    return state_.scalar(state_.word(0x00327cd4)==0?0x003256d0:0x003256d8);
}
void NativeBackend::initialize(Memory& m,const Config& config){
    // Explicit synthetic scenario inputs, not a recovered whole Car constructor:
    // identity orientation, geometric ground placement on a horizontal plane,
    // zero initial motion, recovered Race state4 and no damage/AI/race orchestration.
    const auto cfg=m.allocate(0x560);for(unsigned i=0;i<config.raw.size();++i)m.byte(cfg+i,config.raw[i]);
    car_=m.allocate(0x900);controls_=m.allocate(0x500);mover_=m.allocate(0x80);
    const auto events=m.allocate(0x120),owner=m.allocate(0x20),mode=m.allocate(0x10),query=m.allocate(0x2c0);
    m.word(mode,4);m.word(0x00327258,mode);m.word(car_+4,query);m.word(car_+0xc,cfg);m.word(car_+0x14,owner);m.word(owner+8,1);m.word(car_+0x34,events);
    m.word(car_+0x40,cfg);m.word(car_+0x44,query);m.scalar(car_+0x3e0,1);
    m.word(car_+0x3bc,1);m.word(car_+0x3dc,1);
    float center_height=std::numeric_limits<float>::infinity();
    for(unsigned i=0;i<4;++i){const auto row=cfg+0x120+i*0x20;const float local_z=m.scalar(row+8)-m.scalar(cfg+0x118);center_height=std::min(center_height,plane_.height+m.scalar(row+0xc)-local_z);}
    const auto st=car_+0x40;
    hp2::recovered::CarState(m,st).ResetCar();
    auto orientation=init::bIdentity();std::array<float,4> position{{0,0,center_height,1}};
    if(const auto& track=track_contact_.environment){
        // Debug AddRacingCar00149D78 and Car::FindFlatOnTrackPosition00132788.
        const auto angle=static_cast<std::uint16_t>(hp2::recovered::bATan(m,track->direction[0],track->direction[1]));
        position={{track->start[0],track->start[1],track->start[2],1}};
        hp2::recovered::Car(m,car_).FindFlatOnTrackPosition(orientation,position,angle,*contact());m.half(st+0x364,angle);
        auto geometry=position;for(unsigned i=0;i<3;++i)geometry[i]-=m.scalar(cfg+0x110+i*4);m.vector(st+0x10,geometry);
    }
    m.matrix(st+0x50,orientation);m.vector(st+0x20,position);
    m.word(st+0x388,2);
    hp2::recovered::Sub_0013C750(m);
    hp2::recovered::RealDriver driver(m,controls_);driver.Sub_0013AB98(car_);
    driver.DisableStartLineState();
    // Synthetic playerless fixture; original RealDriver itself controls the vehicle.
    m.word(car_+0x20,1);m.word(car_+0x28,controls_);m.word(controls_+0x234,0);
    // Synthetic environment: no editor and no original replay capture.
    m.word(0x3281d4,0);const auto replay=m.allocate(0x50b4);m.word(0x3271a8,replay);
    // Refresh published wheel matrices before first force/reset traversal.
    for(std::int32_t i=0;i<4;++i)hp2::recovered::CarState(m,st).CalculateWheelMatrices(i);
    auto services=init_services();hp2::recovered::PhysicsCar(m,mover_).Initialize(cfg,controls_,car_,services);
    // Original texture absence stays absent in native-only fixtures. The interactive
    // bridge supplies the original texture lookup before initialization.
    SkidServices skids{&skid_arena_,contact(),{}};
    hp2::recovered::InitSkids(m,skids,skid_lookup_?skid_lookup_:SkidTextureLookup{[](const std::string& name,std::uint32_t hash){return SkidTexture{name,hash,false,false,false};}});
    m.byte(car_+0x4a8,1);m.word(car_+0x514,0xffffffffu);
    // Explicit host-audio activation proxies, not original audio constructors.
    // BaseCarSound+34 -> HornSound3d Activated word0 (E-0082-H01).
    const auto sound=m.allocate(0x50),horn=m.allocate(4);
    m.word(sound+0x20,car_);m.word(sound+0x34,horn);m.word(car_+0x850,sound);
}
Snapshot NativeBackend::publish(Memory& m,float dt)const{
    const auto body=m.word(mover_+0x18),st=car_+0x40;Snapshot out;out.dt=dt;out.surface=plane_.settings;out.surface_catalog=catalog_;
    if(track_contact_.environment){out.track_label=track_contact_.environment->label;out.surface.reset();out.track_material_catalog=track_catalog_;}
    auto transform=m.matrix(body+0x20);const auto position=m.vector(st+0x10);for(unsigned i=0;i<3;++i)transform[12+i]=position[i];transform[15]=1;out.body=transform;
    const auto velocity=m.vector(body+0x90),angular=m.vector(body+0xa0);out.velocity=std::array<float,3>{{velocity[0],velocity[1],velocity[2]}};out.angular_velocity=std::array<float,3>{{angular[0],angular[1],angular[2]}};
    std::array<WheelSnapshot,4> wheels;
    for(unsigned i=0;i<4;++i){const auto w=m.word(mover_+0x30+4*i);auto& dst=wheels[i];dst.tire=m.matrix(st+0xb0+i*0x40);dst.brake=m.matrix(st+0x1b0+i*0x40);dst.blur=dst.brake;dst.phase=m.scalar(w+0x1c);dst.angular_velocity=m.scalar(w+0x20);dst.contact=m.byte(w+0x155)!=0;
        // Tool-only blur opacity; not original physics or a recovered Render implementation.
        float strength=std::abs(dst.angular_velocity)*0.02f;if(1<strength)strength=1;dst.blur_alpha=strength*90.0f;
        dst.grip=observe_grip(m,w);
    }
    out.wheels=wheels;validate_matrix(*out.body);validate_wheels(*out.wheels);
    PowertrainSnapshot power;
    // Debug UpdateCar publishes EngineRPM at State+390 (E-0044-P01).
    power.rpm=m.scalar(st+0x390);power.speed=m.scalar(st+0x384);power.forward_speed=m.scalar(st+0x380);
    power.gear=m.signed_word(st+0x388);power.manual_transmission=m.signed_word(controls_+0x24c);power.handbrake=m.scalar(controls_+0x21c)>0;
    power.horn=horn_activation(m,m.word(car_+0x850));
    if(!std::isfinite(power.rpm)||!std::isfinite(power.speed)||!std::isfinite(power.forward_speed))
        throw std::runtime_error("Non-finite native powertrain publication");
    out.powertrain=power;
    out.lights=vehicle_light_state(m,car_,track_contact_.environment?track_contact_.environment->time_of_day:-1);
    out.flares=vehicle_flare_state(m,car_,track_contact_.environment?track_contact_.environment->time_of_day:-1);
    for(const auto* values:{&*out.velocity,&*out.angular_velocity})
        for(float value:*values)if(!std::isfinite(value))throw std::runtime_error("Non-finite native motion publication");
    if(!std::isfinite(dt)||dt<=0)throw std::runtime_error("Invalid native timestep publication");
    return out;
}
Snapshot NativeBackend::advance(const Config& config,const Input& input,float elapsed,const SurfaceSettings& surface){
    validate_input(input);
    struct ArenaTransaction {
        SkidArena& destination;SkidArena previous;
        SurfaceSettings& surface;SurfaceSettings previous_surface;bool committed=false;
        ~ArenaTransaction(){if(!committed){destination=std::move(previous);surface=previous_surface;}}
    } arena_transaction{skid_arena_,skid_arena_,plane_.settings,plane_.settings};
    validate_surface(surface);
    if(!std::isfinite(elapsed)||elapsed<0)throw std::invalid_argument("Elapsed simulation clock input must be finite and nonnegative");
    if(!loaded_){auto image=read_debug_image(elf_);auto catalog=read_surface_catalog(image);auto track_catalog=read_track_material_catalog(image);image_=std::move(image);catalog_=std::move(catalog);track_catalog_=std::move(track_catalog);state_=image_;loaded_=true;}
    // This provider is only an input seam; committed settings are published with the candidate.
    // A failed call leaves the committed Memory and externally held snapshot unchanged.
    plane_.settings=surface;
    // A failed constructor/step cannot commit partial memory or random state.
    Memory next=state_;
    StaticCollisions next_collisions=collisions_;
    auto next_sound_selector=collision_sound_selector_;
    auto next_tire_mixer=tire_audio_mixer_;
    auto next_splash_mixer=splash_audio_mixer_;
    if(!initialized_){
        hp2::recovered::Sub_0019B358(next);hp2::recovered::Sub_00139510(next);hp2::recovered::ResetWorldTime(next);hp2::recovered::Sub_001A2FC0(next);
        // Debug00254350 stores the homogeneous identity addend (E-0044-M02).
        hp2::recovered::Sub_00254350(next);
        initialize(next,config);
        if(track_contact_.environment){if(!collision_geometry_available_)throw std::runtime_error("Original car collision geometry was not supplied");const auto cfg=next.word(car_+0xc);next_collisions.configure(next,config,{{next.scalar(cfg+0x110),next.scalar(cfg+0x114),next.scalar(cfg+0x118)}},collision_geometry_);}
    }
    native::ScheduleServices schedule;
    std::vector<std::uint32_t> observations;
    schedule.Sub_00183498=[&](std::uint32_t value,std::uint32_t bits,std::uint32_t kind){observations.insert(observations.end(),{value,bits,kind});};
    hp2::recovered::PrepareRealTimestep(next,elapsed,schedule);const float dt=next.scalar(0x00327ce4);
    hp2::recovered::AdvanceRealTime(next);hp2::recovered::PrepareWorldTimestep(next,dt);
    hp2::recovered::RealDriver driver(next,controls_);
    if(input.horn!=horn_input_)driver.JoyHandler_HonkHorn(0,0,input.horn?1:0);
    if(input.transmission>=0)driver.SetManualTransmission(input.transmission);
    if(input.shift>0)driver.JoyHandler_ShiftUp(0,0,1);
    if(input.shift<0)driver.JoyHandler_ShiftDown(0,0,1);
    if(input.neutral&&next.word(controls_+0x24c)==1)driver.SetDesiredGear(1);
    // Semantic normalized inputs enter recovered named RealDriver inputs, not outputs.
    next.scalar(controls_+0x25c,input.throttle);next.scalar(controls_+0x264,input.brake);
    next.scalar(controls_+0x254,input.steering);next.scalar(controls_+0x274,input.handbrake);
    PlayerServices player;
    // Debug001C2ED0 null pTrackRouteEditor branch and Debug001FEE80 empty body.
    player.MaybeAddTrackRoutePoint=[](Memory& m,std::uint32_t car){hp2::recovered::MaybeAddTrackRoutePoint(m,car);};
    player.RealDriverAddNearbyDebugTopology=[](Memory& m,std::uint32_t car){hp2::recovered::RealDriverAddNearbyDebugTopology(m,car);};
    player.IsJoystickPluggedIn=[](std::int32_t){return true;};
    player.IsJoystickTypeWheel=[](std::int32_t){return false;};
    // Debug0013E070 CapturingFlag0 branch: original replay capture is disabled in
    // this fixture. Session recording is separate port input recording.
    player.RegisterInterestingEvent=[](Memory& m,float value,const char* name){hp2::recovered::ReplayManager(m,m.word(0x3271a8)).RegisterInterestingEvent(value,name);};
    driver.Drive(dt,player);
    // E-0081-L01: one frame history/terrain window around all collision retries.
    // Full topology/path/Player lifecycle is outside this explicit state slice.
    begin_movement_state(next,car_,dt);
    auto init=init_services();auto services=driving_services(init);const auto body=next.word(mover_+0x18);
    CollisionMotion motion;
    const auto move=[&](float timestep){
        motion=hp2::recovered::DynamicCollisionBody::BeginMove(next,body,timestep);
        hp2::recovered::PhysicsCar(next,mover_).Move(timestep,services);
        // Car::Move00131B90 consumes ResetMe after each mover call. Route-point
        // ownership is not yet bound; never silently ignore a recovery request.
        if(next.word(car_+0x854)!=0)
            hp2::not_implemented("0x00132FB0","Deferred route reset requires TrackedController and distance-aware route ownership");
    };
    move(dt);
    if(const auto& track=track_contact_.environment){const auto recompute=[&]{hp2::recovered::PhysicsCar(next,mover_).ComputeAuxiliaries(init);};next_collisions.process(next,body,car_,track->topology,motion,move,recompute);}
    finish_movement_state(next,car_,dt);
    // E0089W01/W03: once per candidate native frame; retries and visual captures
    // cannot publish sound/RNG/timed-wiper state until this candidate commits.
    trigger_water_terrain_state(next,car_,0,next_splash_mixer,[&](float value,const char* name){player.RegisterInterestingEvent(next,value,name);});
    auto snapshot=publish(next,dt);
    snapshot.wipers=wiper_snapshot(next,car_);
    snapshot.splash_audio=next_splash_mixer.update(next);
    snapshot.tire_audio=next_tire_mixer.update(next,car_);
    // Port own-car listener binding, not original camera/world ownership.
    const auto listener=next.vector(body+0x10);
    // CollisionList appends at construction00219230; polygon map order is unrelated.
    std::vector<CollisionAudioContact*> audio_order;
    for(auto& entry:next_collisions.audio_contacts)audio_order.push_back(&entry.second);
    std::sort(audio_order.begin(),audio_order.end(),[](const auto* left,const auto* right){return left->id<right->id;});
    for(auto* entry:audio_order){
        if(auto scrape=update_collision_scrape(*entry,next,{{listener[0],listener[1],listener[2]}}))snapshot.collision_scrapes.push_back(*scrape);
        auto request=update_collision_audio(*entry,next_sound_selector,next,{{listener[0],listener[1],listener[2]}});if(request)snapshot.collision_sounds.push_back(*request);}
    for(auto it=next_collisions.fading_scrapes.begin();it!=next_collisions.fading_scrapes.end();){
        it->voice.volume=collision_scrape_fade_step(it->voice.volume,it->adjust);
        if(collision_scrape_fade_finished(next,it->voice.volume))it=next_collisions.fading_scrapes.erase(it);
        else{snapshot.collision_scrapes.push_back(it->voice);++it;}
    }
    hp2::recovered::AdvanceWorldTime(next);collision_sound_selector_=next_sound_selector;tire_audio_mixer_=next_tire_mixer;splash_audio_mixer_=next_splash_mixer;snapshot.step=step_+1;state_=std::move(next);collisions_=std::move(next_collisions);initialized_=true;horn_input_=input.horn;++step_;arena_transaction.committed=true;return snapshot;
}
unsigned NativeBackend::terrain_at_body(Memory& m,std::uint32_t car){
 Memory::Scratch scratch(m);const auto terrain=m.allocate(4);
 contact()->GetElevation(m,car,car+0x60,terrain,0,0);
 return m.word(terrain);
}
std::vector<SkidQuad> NativeBackend::render_skids(const SkidView& view){
    if(!initialized_)return {};
    Memory candidate=state_;std::vector<SkidQuad> commands;
    SkidServices services{&skid_arena_,contact(),[&](const SkidQuad& command){commands.push_back(command);}};
    hp2::recovered::RenderSkids(candidate,view,services);
    state_=std::move(candidate);return commands;
}
}

