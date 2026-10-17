#include "game_core/Sub_00137D90.hpp"
#include "game_core/Sub_00137100.hpp"
#include "game_core/port/not_implemented.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace port::driving;
using namespace port::driving::native;
namespace {
void check(bool b,const char* why){if(!b)throw std::runtime_error(why);}
std::uint32_t ga(std::uint16_t off){return 0x3358f0u+static_cast<std::int16_t>(off);}
struct Fixture {
    Memory m;DrivingServices services;
    std::uint32_t s,car,state,config,controls,body,engine,drive,helper,steering,mode;
    unsigned integrated=0,normalized=0,published=0,clocks=0,deltas=0;float total_dt=0;
    Fixture() {
        m.map(0x310000,0x30000);m.map(0x2f0500,0x80);
        s=m.allocate(0x78);car=m.allocate(0x8a0);state=car+0x40;config=m.allocate(0x560);
        controls=m.allocate(0x900);body=m.allocate(0x180);engine=m.allocate(0x60);drive=m.allocate(0xa0);
        helper=m.allocate(0x18);steering=m.allocate(0x90);mode=m.allocate(4);
        m.word(s,controls);m.word(s+0x18,body);m.word(s+0x1c,steering);m.word(s+0x40,engine);
        m.word(s+0x44,drive);m.word(s+0x48,helper);m.word(s+0x4c,config);m.word(s+0x50,state);m.word(s+0x54,car);
        m.word(body+0xc,config+0x1e0);m.scalar(config+0x1e0,1);m.matrix(config+0x1f0,init::Sub_0025AC68());
        m.matrix(body+0x20,init::Sub_0025AC68());m.matrix(body+0xb0,init::Sub_0025AC68());
        m.scalar(config+0x120,1);m.scalar(config+0x180,-1);
        const auto steer_table=m.allocate(0x20);m.word(steering,config);m.word(steering+8,steer_table);m.word(steer_table+0x14,0x1a5d60);
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
        m.scalar(ga(0x8dbc),1);m.scalar(ga(0x8dc0),2);m.scalar(ga(0x8dc4),4);m.scalar(ga(0x8dc8),1);
        // Synthetic polynomial coefficients: f(a)=a on the exercised0/1 nodes.
        // These are algorithm fixtures, not original trigonometric values.
        m.scalar(ga(0x98c4),1);m.scalar(ga(0x98c8),1000);m.scalar(ga(0x98d0),1000);m.scalar(ga(0x98e8),1);
        m.scalar(car+0x3e0,1);m.scalar(s+0x70,1);m.scalar(s+0x74,1);m.word(s+0x60,1);m.word(s+0x64,1);m.word(s+0x68,2);
        m.scalar(controls+0x214,1);m.word(controls+0x220,2);m.word(controls+0x224,2);
        services.Field_001389A4_stack_w=123;
        services.Sub_00250F38=[&](){return std::uint64_t(++clocks);};
        services.Sub_00250F48=[&](std::uint64_t a,std::uint64_t b){check(a<b,"Performance callbacks ordered");++deltas;return std::uint32_t(b-a);};
        services.Sub_0019C290=[&](Memory& target,std::uint32_t list,float dt){
            check(&target==&m && list==s+8,"Integrator uses original component-list address");
            ++integrated;total_dt+=dt;m.vector(body+0x60,{{8,4,2,99}});
        };
        services.Sub_0011BD90=[&](Memory&,std::uint32_t matrix){check(matrix==body+0x20,"Normalization matrix address");++normalized;};
        services.Sub_00139188=[&](Memory&,std::uint32_t mover,float dt){check(mover==s && dt==0.25f,"Final original publication arguments");++published;};
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
    check(f.m.matrix(f.steering+0x10)==init::Sub_0025AC68() && f.m.matrix(f.steering+0x50)==init::Sub_0025AC68(),"Zero steering creates two matrices with synthetic exact nodes");
    const auto old_seed=0u;const auto mix=old_seed^0x1d872b41u;const auto x=mix^(mix>>5);
    check(f.m.word(ga(0xc778))==(x^mix^(x<<27)),"RNG update uses original unsigned bit operations");
    Fixture stopped;stopped.m.scalar(stopped.controls+0x214,0);
    Move(stopped.m,stopped.s,0.25f,stopped.services);
    check(stopped.m.scalar(stopped.body+0x60)==0 && stopped.m.scalar(stopped.body+0x64)==0 && stopped.m.scalar(stopped.body+0x68)==0,"Freeze XYZ and explicit W bits");
    check(stopped.m.word(stopped.body+0x6c)==123,"Freeze explicit original uninitialized W projection");
    Fixture missing;missing.services.Sub_0019C290={};bool caught=false;
    try{Move(missing.m,missing.s,0.25f,missing.services);}catch(const hp2::NotImplemented&){caught=true;}
    check(caught,"Missing integration does not silently succeed");
    Fixture running;running.m.word(running.mode,4);running.m.scalar(ga(0x8448),0.25f);
    running.m.word(ga(0xb388),8);running.services.Field_001389A4_stack_w.reset();
    Move(running.m,running.s,0.25f,running.services);
    check(running.m.word(running.s+0x60)==1 && running.m.scalar(running.body+0x60)==8,"Race4 running skips launch/freeze despite preserved constructor flag");
    check(running.m.word(running.s+0x68)==2 && running.m.word(ga(0xc778))==0,"Race4 running does not consume launch RNG");
    check(running.m.scalar(ga(0xa784))==2 && running.m.scalar(ga(0xa794))==1 && running.m.scalar(ga(0xa79c))==1,"Race4 ordinary telemetry stores");
    Fixture unsupported;unsupported.m.word(unsupported.mode,4);unsupported.m.word(ga(0xa758),1);caught=false;
    try{Move(unsupported.m,unsupported.s,0.25f,unsupported.services);}catch(const hp2::NotImplemented&){caught=true;}
    check(caught && unsupported.clocks==0,"Excluded Race4 diagnostic flag rejected before original mutation");
    Memory rng;rng.map(0x1000,4);rng.word(0x1000,0xffffffffu);
    check(Sub_0025A3D0(rng,0x1000,0x7fffffffu)==1,"RNG uses DIVU counterevidence over signed decompile");
    std::cout<<"DRIVING_MOVER_OK: control and service instruction fixtures, original validation deferred\n";
}
