#include "game_core/indep/inc/PhysicsCar.hpp"
#include "game_core/indep/src/carstate.hpp"
#include "game_core/indep/inc/Car.hpp"
#include "game_core/port/driving/debug_globals.hpp"
#include "game_core/port/driving/projections.hpp"
#include "game_core/port/driving/projections.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace port::driving;
using namespace port::driving::native;
namespace {
void check(bool b,const char* why){if(!b)throw std::runtime_error(why);}
std::uint32_t ga(std::uint16_t off){return debug_global_address(0x3358f0u+static_cast<std::int16_t>(off));}
struct Fixture {
    Memory m;DrivingServices services;
    std::uint32_t s,car,state,config,controls,body,engine,drive,helper,steering,mode;
    unsigned integrated=0,normalized=0,published=0,clocks=0,deltas=0;float total_dt=0;
    Fixture() {
        m.map(0x310000,0x30000);m.map(0x2e7d90,0x80);
        s=m.allocate(0x78);car=m.allocate(0x8a0);state=car+0x40;config=m.allocate(0x560);
        controls=m.allocate(0x900);body=m.allocate(0x180);engine=m.allocate(0x60);drive=m.allocate(0xa0);
        helper=m.allocate(0x18);steering=m.allocate(0x90);mode=m.allocate(4);
        m.word(s,controls);m.word(s+0x18,body);m.word(s+0x1c,steering);m.word(s+0x40,engine);
        m.word(s+0x44,drive);m.word(s+0x48,helper);m.word(s+0x4c,config);m.word(s+0x50,state);m.word(s+0x54,car);
        m.word(body+0xc,config+0x1e0);m.scalar(config+0x1e0,1);m.matrix(config+0x1f0,init::bIdentity());
        m.matrix(body+0x20,init::bIdentity());m.matrix(body+0xb0,init::bIdentity());
        m.scalar(config+0x120,1);m.scalar(config+0x180,-1);
        const auto steer_table=m.allocate(0x20);m.word(steering,config);m.word(steering+8,steer_table);m.word(steer_table+0x14,0x1a3540);
        m.word(engine+0x10,config+0x2b0);m.word(engine+0x14,state);m.scalar(engine+0xc,100);
        m.word(drive+0x10,config+0x270);m.word(drive+0x14,engine);m.word(drive+0x28,state);
        m.word(drive+0x30,2);m.word(drive+0x38,3);m.word(drive+0x48,2);m.word(drive+0x2c,2);
        m.word(config+0x288,5);m.word(helper,config+0x2f0);
        const auto car_aux=m.allocate(0x200),car_type=m.allocate(16);
        m.word(car+0x34,car_aux);m.word(car+0x14,car_type);m.word(car_type+8,3);
        for(unsigned i=0;i<4;++i){
            const auto wheel=m.allocate(0x190),controller=m.allocate(0xb0);
            m.word(wheel+0xc,config+0x1a0);m.word(controller+0x90,wheel);m.word(controller+0x94,body);
            m.word(s+0x30+4*i,wheel);m.word(s+0x20+4*i,controller);m.word(drive+0x18+4*i,wheel);
        }
        m.word(ga(0xa900),mode);m.word(mode,2);
        m.scalar(ga(0xa728),0.125f);m.scalar(ga(0xa724),1);m.scalar(ga(0x8420),1);m.scalar(ga(0x8428),0.25f);
        m.scalar(0x00325780,1);m.scalar(0x00325784,2);m.scalar(0x00325788,4);m.scalar(0x0032578c,1);
        // Synthetic polynomial coefficients: f(a)=a on the exercised0/1 nodes.
        // These are algorithm fixtures, not original trigonometric values.
        m.scalar(0x0032625c,1);m.scalar(0x00326260,1000);m.scalar(0x00326268,1000);m.scalar(0x00326280,1);
        m.scalar(0x00324dd0,0.34906584f);m.scalar(0x00327064,1);m.scalar(0x0032706c,30);m.scalar(0x00327070,0.7f);
        m.scalar(car+0x3e0,1);m.scalar(s+0x70,1);m.scalar(s+0x74,1);m.word(s+0x60,1);m.word(s+0x64,1);m.word(s+0x68,2);
        m.scalar(controls+0x214,1);m.word(controls+0x220,2);m.word(controls+0x224,2);
        services.Field_001389A4_stack_w=123;
        services.Sub_00250F38=[&](){return std::uint64_t(++clocks);};
        services.Sub_00250F48=[&](std::uint64_t a,std::uint64_t b){check(a<b,"Performance callbacks ordered");++deltas;return std::uint32_t(b-a);};
        services.IntegrateEuler=[&](Memory& target,std::uint32_t list,float dt){
            check(&target==&m && list==s+8,"Integrator uses original component-list address");
            ++integrated;total_dt+=dt;m.vector(body+0x60,{{8,4,2,99}});
        };
        services.eOrthonormalizeMatrix=[&](Memory&,std::uint32_t matrix){check(matrix==body+0x20,"Normalization matrix address");++normalized;};
        services.UpdateCar=[&](Memory&,std::uint32_t mover,float dt){check(mover==s && dt==0.25f,"Final original publication arguments");++published;};
    }
};
}
int main(){
    Fixture f;check(Move(f.m,f.s,0.25f,f.services)==0.25f,"Mover returns original dt");
    check(f.integrated==3 && f.normalized==3 && f.published==1 && f.deltas==4,"Recovered subdivision and service ordering");
    check(std::abs(f.total_dt-0.25f)<0.000001f,"Substeps cover requested dt");
    check(f.m.word(f.s+0x60)==0 && f.m.word(f.s+0x64)==0 && f.m.word(f.s+0x68)==1,"Throttle launch consumes one original RNG counter");
    check(f.m.word(f.s+0x58)==1000 && f.m.word(f.s+0x5c)==0,"Launch timing CVT.W.S ordering");
    check(f.m.scalar(f.body+0x60)==8 && f.m.scalar(f.engine+0x44)==1,"Launch keeps integrator output and copies normalized control");
    check(f.m.matrix(f.steering+0x10)==init::bIdentity() && f.m.matrix(f.steering+0x50)==init::bIdentity(),"Zero steering creates two matrices with synthetic exact nodes");
    const auto old_seed=0u;const auto mix=old_seed^0x1d872b41u;const auto x=mix^(mix>>5);
    check(f.m.word(0x00329060)==(x^mix^(x<<27)),"RNG update uses original unsigned bit operations");
    Fixture stopped;stopped.m.scalar(stopped.controls+0x214,0);
    Move(stopped.m,stopped.s,0.25f,stopped.services);
    check(stopped.m.scalar(stopped.body+0x60)==0 && stopped.m.scalar(stopped.body+0x64)==0 && stopped.m.scalar(stopped.body+0x68)==0,"Freeze XYZ and explicit W bits");
    check(stopped.m.word(stopped.body+0x6c)==123,"Freeze explicit original uninitialized W projection");
    Fixture missing;missing.services.IntegrateEuler={};bool caught=false;
    try{Move(missing.m,missing.s,0.25f,missing.services);}catch(const hp2::NotImplemented&){caught=true;}
    check(caught,"Missing integration does not silently succeed");
    Fixture running;running.m.word(running.mode,4);running.m.scalar(ga(0x8448),0.25f);
    running.m.word(ga(0xb388),8);running.services.Field_001389A4_stack_w.reset();
    Move(running.m,running.s,0.25f,running.services);
    check(running.m.word(running.s+0x60)==1 && running.m.scalar(running.body+0x60)==8,"Race4 running skips launch/freeze despite preserved constructor flag");
    check(running.m.word(running.s+0x68)==2 && running.m.word(0x00329060)==0,"Race4 running does not consume launch RNG");
    check(running.m.scalar(ga(0xa784))==2 && running.m.scalar(ga(0xa794))==1 && running.m.scalar(ga(0xa79c))==1,"Race4 ordinary telemetry stores");
    Fixture unsupported;unsupported.m.word(unsupported.mode,4);unsupported.m.word(ga(0xa758),1);caught=false;
    try{Move(unsupported.m,unsupported.s,0.25f,unsupported.services);}catch(const hp2::NotImplemented&){caught=true;}
    check(caught && unsupported.clocks==0,"Excluded Race4 diagnostic flag rejected before original mutation");
    Memory rng;rng.map(0x1000,4);rng.word(0x1000,0xffffffffu);
    check(bRandom(rng,0x1000,0x7fffffffu)==1,"RNG uses DIVU counterevidence over signed decompile");
    // Debug-specific regression: SetBoost no longer scales engine boost for damaged AI.
    Fixture boost;boost.m.byte(boost.car+0x848,1);boost.m.byte(boost.m.word(boost.car+0x34)+0x12,1);
    hp2::recovered::PhysicsCar canonical(boost.m,boost.s);canonical.SetBoost(2);
    check(boost.m.scalar(boost.engine+0x58)==2&&boost.m.scalar(boost.body+0x178)==2,"Debug SetBoost preserves requested boost for damaged AI");
    boost.m.scalar(boost.config+0x30c,0.625f);check(canonical.GetBrakeSettings(0)==1.25f&&canonical.GetBrakeSettings(3)==0.75f,"Debug BrakeBias uses Control+0xc and rear double subtraction");
    // Wheel/brake translations differ in the original Debug function.
    boost.m.word(boost.state,boost.config);boost.m.scalar(0x00324b1c,1000);boost.m.scalar(0x00324b20,1000);boost.m.scalar(0x00324b24,1000);boost.m.scalar(0x00324b28,1000);boost.m.scalar(0x00324b48,1);
    const auto row=boost.config+0x160;boost.m.vector(row,{{1,2,3,0.5f}});boost.m.scalar(row+0x18,0.4f);boost.m.scalar(boost.state+0x308,0.75f);
    hp2::recovered::CarState published(boost.m,boost.state);published.CalculateWheelMatrices(2);
    const auto tire=boost.m.matrix(boost.state+0x130),brake=boost.m.matrix(boost.state+0x230);
    check(tire[12]==1&&tire[13]==2.4f&&tire[14]==3.75f&&brake[12]==1&&brake[13]==2&&brake[14]==3,"Only tire translation receives render/suspension offsets");
    published.SetWheelTouchingGround(2,1);published.SetWheelTouchingGround(2,1);published.SetWheelTouchingGround(2,0);check(boost.m.word(boost.state+0x37c)==0,"Contact count changes on transitions only");
    hp2::recovered::Car logical_car(boost.m,boost.car);boost.m.word(boost.car+0x858,1);boost.m.word(0x00327cf8,2);boost.m.scalar(0x00324d24,1);boost.m.scalar(boost.car+0x85c,2);check(logical_car.IsBlinking(),"Blinking uses Debug858/85c members");
    // E-0046-P01: synthetic values exercise instruction branches, not original
    // trajectories. Native speed below5 takes original GetYaw's exact zero branch.
    auto player=[](Fixture& p){p.m.word(p.car+0x20,1);p.m.word(p.mode,4);p.m.word(p.state+0x39c,1);
        p.services.IsJoystickTypeWheel=[](std::int32_t port){check(port==0,"Original RealDriver Port passed to classification");return false;};
        p.m.scalar(p.state+0x380,1);p.m.scalar(ga(0x8400),1);
        p.m.scalar(0x00327084,10);p.m.scalar(0x00327088,20);p.m.scalar(0x0032708c,5);
        p.m.scalar(0x00327090,10);p.m.scalar(0x00327094,0.6f);p.m.scalar(0x00327098,2);
        p.m.scalar(0x0032709c,0.5f);p.m.scalar(0x003270a0,1);p.m.scalar(0x003270a4,2);
        p.m.scalar(0x00327d08,0.25f);p.m.scalar(0x003102e8+0x20,1);
    };
    Fixture burnout;player(burnout);burnout.m.scalar(burnout.m.word(burnout.s+0x38)+0x108,15);
    Move(burnout.m,burnout.s,0.25f,burnout.services);
    check(burnout.m.word(burnout.state+0x3a4)==1&&burnout.m.scalar(burnout.state+0x3a8)==2,"Rear left torque selects original burnout side and minimum time");
    const float expected=0.6f+(1.0f/10.0f)*(1.0f-0.5f)*(1.0f-0.6f);
    check(burnout.m.scalar(burnout.state+0x3ac)==expected,"Burnout traction retains DIV/sub/multiply/add order");
    Fixture timed;player(timed);timed.m.word(timed.state+0x3a4,2);timed.m.scalar(timed.state+0x3a8,1);
    timed.m.scalar(timed.state+0x3b0,0.25f);Move(timed.m,timed.s,0.25f,timed.services);
    check(timed.m.word(timed.state+0x3a4)==2&&timed.m.scalar(timed.state+0x3b0)==0.5f,"Allow threshold equality keeps burnout and accumulator");
    Move(timed.m,timed.s,0.25f,timed.services);check(timed.m.word(timed.state+0x3a4)==0,"Strict allow threshold cancels on next step");
    Fixture wheel_device;player(wheel_device);wheel_device.services.IsJoystickTypeWheel=[](std::int32_t){return true;};
    wheel_device.m.word(wheel_device.state+0x3a4,1);wheel_device.m.scalar(wheel_device.state+0x3a8,1);
    Move(wheel_device.m,wheel_device.s,0.25f,wheel_device.services);
    check(wheel_device.m.scalar(wheel_device.state+0x3a8)==1,"Wheel-type controller skips burnout processing");
    std::cout<<"DRIVING_MOVER_OK: control and service instruction fixtures, original validation deferred\n";
}
