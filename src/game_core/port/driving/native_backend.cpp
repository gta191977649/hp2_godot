#include "native_backend.hpp"
#include "game_core/Sub_0018F808.hpp"
#include "game_core/Sub_001A4930.hpp"
#include "game_core/Sub_0019DA30.hpp"
#include "game_core/port/not_implemented.hpp"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <limits>
namespace port::driving {
native::InitServices NativeBackend::init_services(){
    native::InitServices s;s.contact=&plane_;s.Field_00137938_stack_w=0;
    s.Sub_00250F38=[] {return static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());};
    s.Sub_00250F48=[](std::uint64_t start,std::uint64_t end){return static_cast<std::uint32_t>(end-start);};
    s.ResolveForces=[](Memory& m,std::uint32_t a){native::ResolveForces(m,a);};
    // The fixture does not install an external race/police predicate service.
    return s;
}
native::DrivingServices NativeBackend::driving_services(native::InitServices& init){
    native::DrivingServices s;s.Sub_00250F38=init.Sub_00250F38;s.Sub_00250F48=init.Sub_00250F48;s.Field_001389A4_stack_w=0;
    s.Sub_0019C290=[&init](Memory& m,std::uint32_t a,float dt){native::integrate(m,a,dt,init);};
    s.Sub_0011BD90=[](Memory& m,std::uint32_t a){native::Sub_0011BD90(m,a);};
    s.Sub_00139188=[](Memory& m,std::uint32_t a,float dt){native::Sub_00139188(m,a,dt);};
    return s;
}
void NativeBackend::reset(){initialized_=false;step_=0;if(loaded_)state_=image_;}
float NativeBackend::minimum_frame_elapsed()const{
    if(!loaded_)return 0;
    // Host render pacing uses the lower bound of recovered001A33F0, not a new timestep.
    return state_.scalar(state_.word(0x00330c54)==0?0x0032e5fc:0x0032e604);
}
void NativeBackend::initialize(Memory& m,const Config& config){
    // Explicit synthetic scenario inputs, not a recovered whole Car constructor:
    // identity orientation, geometric ground placement on a horizontal plane,
    // zero initial motion, recovered Race state4 and no damage/AI/race orchestration.
    const auto cfg=m.allocate(0x560);for(unsigned i=0;i<config.raw.size();++i)m.byte(cfg+i,config.raw[i]);
    car_=m.allocate(0x900);controls_=m.allocate(0x500);mover_=m.allocate(0x80);
    const auto events=m.allocate(0x120),owner=m.allocate(0x20),mode=m.allocate(0x10),query=m.allocate(0x2c0);
    m.word(mode,4);m.word(0x003301f0,mode);m.word(car_+4,query);m.word(car_+0xc,cfg);m.word(car_+0x14,owner);m.word(owner+8,1);m.word(car_+0x34,events);
    m.word(car_+0x40,cfg);m.word(car_+0x44,query);m.scalar(car_+0x3e0,1);
    m.word(car_+0x3bc,1);m.word(car_+0x3dc,1);
    float center_height=std::numeric_limits<float>::infinity();
    for(unsigned i=0;i<4;++i){const auto row=cfg+0x120+i*0x20;const float local_z=m.scalar(row+8)-m.scalar(cfg+0x118);center_height=std::min(center_height,plane_.height+m.scalar(row+0xc)-local_z);}
    const auto st=car_+0x40;m.matrix(st+0x50,init::Sub_0025AC68());m.vector(st+0x20,{{0,0,center_height,1}});
    m.word(st+0x388,2);m.word(controls_+0x220,0xffffffffu);m.word(controls_+0x224,1);
    // Refresh published wheel matrices before first force/reset traversal.
    for(std::int32_t i=0;i<4;++i)Sub_001C0D78(m,st,i);
    auto services=init_services();native::Sub_00137100(m,mover_,cfg,controls_,car_,services);
}
Snapshot NativeBackend::publish(Memory& m,float dt)const{
    const auto body=m.word(mover_+0x18),st=car_+0x40;Snapshot out;out.dt=dt;
    auto transform=m.matrix(body+0x20);const auto position=m.vector(st+0x10);for(unsigned i=0;i<3;++i)transform[12+i]=position[i];transform[15]=1;out.body=transform;
    const auto velocity=m.vector(body+0x90),angular=m.vector(body+0xa0);out.velocity=std::array<float,3>{{velocity[0],velocity[1],velocity[2]}};out.angular_velocity=std::array<float,3>{{angular[0],angular[1],angular[2]}};
    std::array<WheelSnapshot,4> wheels;
    for(unsigned i=0;i<4;++i){const auto w=m.word(mover_+0x30+4*i);auto& dst=wheels[i];dst.tire=m.matrix(st+0xb0+i*0x40);dst.brake=m.matrix(st+0x1b0+i*0x40);dst.blur=dst.brake;dst.phase=m.scalar(w+0x1c);dst.angular_velocity=m.scalar(w+0x20);dst.contact=m.byte(w+0x155)!=0;
        float strength=std::abs(dst.angular_velocity)*0.02f;if(1<strength)strength=1;dst.blur_alpha=strength*90.0f;
    }
    out.wheels=wheels;validate_matrix(*out.body);validate_wheels(*out.wheels);
    for(const auto* values:{&*out.velocity,&*out.angular_velocity})
        for(float value:*values)if(!std::isfinite(value))throw std::runtime_error("Non-finite native motion publication");
    if(!std::isfinite(dt)||dt<=0)throw std::runtime_error("Invalid native timestep publication");
    return out;
}
Snapshot NativeBackend::advance(const Config& config,const Input& input,float elapsed){
    if(!std::isfinite(elapsed)||elapsed<0)throw std::invalid_argument("Elapsed simulation clock input must be finite and nonnegative");
    if(!loaded_){image_=read_original_image(elf_);state_=image_;loaded_=true;}
    // A failed constructor/step cannot commit partial memory or random state.
    Memory next=state_;
    if(!initialized_){
        native::Sub_0019DA30(next);native::Sub_001395B0(next);native::Sub_001A3598(next);Sub_001A57E0(next);
        // Original0025AFA8 stores the homogeneous identity addend.
        Sub_0025AFA8(next);
        initialize(next,config);
    }
    next.scalar(controls_+0x214,input.throttle);next.scalar(controls_+0x218,input.brake);next.scalar(controls_+0x21c,0);
    // Tool steering maps a normalized request to the original raw16-bit angle.
    // The final scale is supplied by original steering conversion0032E6AC.
    // Half a radian is the tool's selectable maximum request, not an original controller curve.
    constexpr float port_steering_range=0.5f;
    const float radians=input.steering*port_steering_range;const float scale=next.scalar(0x0032e6ac);
    next.half(controls_+0x210,static_cast<std::uint16_t>(cvt_w_s(radians/scale)));
    native::ScheduleServices schedule;
    std::vector<std::uint32_t> observations;
    schedule.Sub_00183498=[&](std::uint32_t value,std::uint32_t bits,std::uint32_t kind){observations.insert(observations.end(),{value,bits,kind});};
    native::Sub_001A33F0(next,elapsed,schedule);const float dt=next.scalar(0x00330c64);
    native::Sub_001A3500(next);native::Sub_001A35E0(next,dt);
    auto init=init_services();auto services=driving_services(init);native::Move(next,mover_,dt,services);
    auto snapshot=publish(next,dt);native::Sub_001A35F0(next);snapshot.step=step_+1;state_=std::move(next);initialized_=true;++step_;return snapshot;
}
}

